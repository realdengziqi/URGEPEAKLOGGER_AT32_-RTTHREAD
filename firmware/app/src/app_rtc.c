#include "app_rtc.h"

#include "at32f403a_407_crm.h"
#include "at32f403a_407_gpio.h"

#define APP_RTC_SECONDS_PER_DAY          86400U
#define APP_RTC_YEAR_BASE                2000U

#define DS1302_RST_PORT                  GPIOA
#define DS1302_RST_PIN                   GPIO_PINS_12
#define DS1302_IO_PORT                   GPIOC
#define DS1302_IO_PIN                    GPIO_PINS_12
#define DS1302_SCLK_PORT                 GPIOD
#define DS1302_SCLK_PIN                  GPIO_PINS_2

#define DS1302_CMD_SEC_WRITE             0x80U
#define DS1302_CMD_SEC_READ              0x81U
#define DS1302_CMD_MIN_WRITE             0x82U
#define DS1302_CMD_MIN_READ              0x83U
#define DS1302_CMD_HOUR_WRITE            0x84U
#define DS1302_CMD_HOUR_READ             0x85U
#define DS1302_CMD_DATE_WRITE            0x86U
#define DS1302_CMD_DATE_READ             0x87U
#define DS1302_CMD_MONTH_WRITE           0x88U
#define DS1302_CMD_MONTH_READ            0x89U
#define DS1302_CMD_DAY_WRITE             0x8AU
#define DS1302_CMD_DAY_READ              0x8BU
#define DS1302_CMD_YEAR_WRITE            0x8CU
#define DS1302_CMD_YEAR_READ             0x8DU
#define DS1302_CMD_WP_WRITE              0x8EU
#define DS1302_CMD_WP_READ               0x8FU
#define DS1302_CLOCK_HALT_BIT            0x80U
#define DS1302_WRITE_PROTECT_BIT         0x80U

static const rt_uint8_t app_rtc_days_in_month[12] =
{
    31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U
};

static rt_bool_t app_rtc_initialized = RT_FALSE;
static rt_bool_t app_rtc_valid = RT_FALSE;
static rt_err_t app_rtc_status = -RT_ERROR;
static rt_mutex_t app_rtc_mutex = RT_NULL;

static void ds1302_delay(void)
{
    volatile rt_uint8_t i;

    for (i = 0U; i < 8U; i++)
    {
    }
}

static rt_bool_t app_rtc_is_leap_year(rt_uint16_t year)
{
    if ((year % 4U) != 0U)
    {
        return RT_FALSE;
    }

    if ((year % 100U) != 0U)
    {
        return RT_TRUE;
    }

    return ((year % 400U) == 0U) ? RT_TRUE : RT_FALSE;
}

static rt_uint8_t app_rtc_month_days(rt_uint16_t year, rt_uint8_t month)
{
    if (month < 1U || month > 12U)
    {
        return 0U;
    }

    if (month == 2U && app_rtc_is_leap_year(year))
    {
        return 29U;
    }

    return app_rtc_days_in_month[month - 1U];
}

static rt_bool_t app_rtc_datetime_is_valid(const app_rtc_datetime_t *datetime)
{
    if (datetime == RT_NULL)
    {
        return RT_FALSE;
    }

    if (datetime->year < 2000U || datetime->year > 2099U ||
        datetime->month < 1U || datetime->month > 12U ||
        datetime->day < 1U || datetime->day > app_rtc_month_days(datetime->year, datetime->month) ||
        datetime->weekday < 1U || datetime->weekday > 7U ||
        datetime->hour > 23U || datetime->minute > 59U || datetime->second > 59U)
    {
        return RT_FALSE;
    }

    return RT_TRUE;
}

static rt_bool_t app_rtc_datetime_fields_are_valid(const app_rtc_datetime_t *datetime)
{
    if (datetime == RT_NULL)
    {
        return RT_FALSE;
    }

    if (datetime->year < 2000U || datetime->year > 2099U ||
        datetime->month < 1U || datetime->month > 12U ||
        datetime->day < 1U || datetime->day > app_rtc_month_days(datetime->year, datetime->month) ||
        datetime->hour > 23U || datetime->minute > 59U || datetime->second > 59U)
    {
        return RT_FALSE;
    }

    return RT_TRUE;
}


static rt_uint8_t app_rtc_calculate_weekday(rt_uint16_t year, rt_uint8_t month, rt_uint8_t day)
{
    rt_uint16_t y;
    rt_uint8_t m;
    rt_uint32_t days = 0U;

    for (y = 2000U; y < year; y++)
    {
        days += app_rtc_is_leap_year(y) ? 366U : 365U;
    }

    for (m = 1U; m < month; m++)
    {
        days += app_rtc_month_days(year, m);
    }

    days += (rt_uint32_t)(day - 1U);

    days = (days + 5U) % 7U;
    return (rt_uint8_t)(days + 1U);
}

static rt_uint8_t ds1302_bin_to_bcd(rt_uint8_t value)
{
    return (rt_uint8_t)(((value / 10U) << 4) | (value % 10U));
}

static rt_uint8_t ds1302_bcd_to_bin(rt_uint8_t value)
{
    return (rt_uint8_t)(((value >> 4) * 10U) + (value & 0x0FU));
}

static void ds1302_rst_write(rt_bool_t high)
{
    gpio_bits_write(DS1302_RST_PORT, DS1302_RST_PIN, high ? TRUE : FALSE);
}

static void ds1302_sclk_write(rt_bool_t high)
{
    gpio_bits_write(DS1302_SCLK_PORT, DS1302_SCLK_PIN, high ? TRUE : FALSE);
}

static void ds1302_io_write(rt_bool_t high)
{
    gpio_bits_write(DS1302_IO_PORT, DS1302_IO_PIN, high ? TRUE : FALSE);
}

static rt_bool_t ds1302_io_read(void)
{
    return (gpio_input_data_bit_read(DS1302_IO_PORT, DS1302_IO_PIN) != RESET) ? RT_TRUE : RT_FALSE;
}

static void ds1302_io_output(void)
{
    gpio_init_type gpio_init_struct;

    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
    gpio_init_struct.gpio_pins = DS1302_IO_PIN;
    gpio_init(DS1302_IO_PORT, &gpio_init_struct);
}

static void ds1302_io_input(void)
{
    gpio_init_type gpio_init_struct;

    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_pull = GPIO_PULL_UP;
    gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
    gpio_init_struct.gpio_pins = DS1302_IO_PIN;
    gpio_init(DS1302_IO_PORT, &gpio_init_struct);
}

static void ds1302_write_byte(rt_uint8_t data)
{
    rt_uint8_t i;

    ds1302_io_output();
    for (i = 0U; i < 8U; i++)
    {
        ds1302_io_write((data & 0x01U) ? RT_TRUE : RT_FALSE);
        ds1302_delay();
        ds1302_sclk_write(RT_TRUE);
        ds1302_delay();
        ds1302_sclk_write(RT_FALSE);
        data >>= 1;
    }
}

static rt_uint8_t ds1302_read_byte(void)
{
    rt_uint8_t i;
    rt_uint8_t data = 0U;

    ds1302_io_input();
    for (i = 0U; i < 8U; i++)
    {
        if (ds1302_io_read() != RT_FALSE)
        {
            data |= (rt_uint8_t)(1U << i);
        }
        ds1302_delay();
        ds1302_sclk_write(RT_TRUE);
        ds1302_delay();
        ds1302_sclk_write(RT_FALSE);
    }

    return data;
}

static void ds1302_write_register(rt_uint8_t command, rt_uint8_t value)
{
    ds1302_rst_write(RT_FALSE);
    ds1302_sclk_write(RT_FALSE);
    ds1302_delay();
    ds1302_rst_write(RT_TRUE);
    ds1302_write_byte(command);
    ds1302_write_byte(value);
    ds1302_rst_write(RT_FALSE);
    ds1302_io_input();
}

static rt_uint8_t ds1302_read_register(rt_uint8_t command)
{
    rt_uint8_t value;

    ds1302_rst_write(RT_FALSE);
    ds1302_sclk_write(RT_FALSE);
    ds1302_delay();
    ds1302_rst_write(RT_TRUE);
    ds1302_write_byte(command);
    value = ds1302_read_byte();
    ds1302_rst_write(RT_FALSE);
    ds1302_io_input();

    return value;
}

static void ds1302_gpio_init(void)
{
    gpio_init_type gpio_init_struct;

    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOC_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOD_PERIPH_CLOCK, TRUE);

    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;

    gpio_init_struct.gpio_pins = DS1302_RST_PIN;
    gpio_init(DS1302_RST_PORT, &gpio_init_struct);

    gpio_init_struct.gpio_pins = DS1302_SCLK_PIN;
    gpio_init(DS1302_SCLK_PORT, &gpio_init_struct);

    ds1302_rst_write(RT_FALSE);
    ds1302_sclk_write(RT_FALSE);
    ds1302_io_input();
}

static rt_err_t app_rtc_take(void)
{
    if (app_rtc_mutex == RT_NULL)
    {
        return -RT_ERROR;
    }

    return rt_mutex_take(app_rtc_mutex, RT_WAITING_FOREVER);
}

static void app_rtc_release(void)
{
    if (app_rtc_mutex != RT_NULL)
    {
        rt_mutex_release(app_rtc_mutex);
    }
}

static rt_err_t app_rtc_read_datetime_locked(app_rtc_datetime_t *datetime)
{
    rt_uint8_t sec_reg;

    if (datetime == RT_NULL)
    {
        return -RT_EINVAL;
    }

    sec_reg = ds1302_read_register(DS1302_CMD_SEC_READ);
    datetime->second = ds1302_bcd_to_bin(sec_reg & 0x7FU);
    datetime->minute = ds1302_bcd_to_bin(ds1302_read_register(DS1302_CMD_MIN_READ) & 0x7FU);
    datetime->hour = ds1302_bcd_to_bin(ds1302_read_register(DS1302_CMD_HOUR_READ) & 0x3FU);
    datetime->day = ds1302_bcd_to_bin(ds1302_read_register(DS1302_CMD_DATE_READ) & 0x3FU);
    datetime->month = ds1302_bcd_to_bin(ds1302_read_register(DS1302_CMD_MONTH_READ) & 0x1FU);
    datetime->weekday = ds1302_bcd_to_bin(ds1302_read_register(DS1302_CMD_DAY_READ) & 0x07U);
    datetime->year = (rt_uint16_t)(APP_RTC_YEAR_BASE + ds1302_bcd_to_bin(ds1302_read_register(DS1302_CMD_YEAR_READ)));

    if ((sec_reg & DS1302_CLOCK_HALT_BIT) != 0U)
    {
        return -RT_ERROR;
    }

    return app_rtc_datetime_is_valid(datetime) ? RT_EOK : -RT_ERROR;
}

rt_err_t app_rtc_datetime_to_unix(const app_rtc_datetime_t *datetime, rt_uint32_t *unix_time)
{
    rt_uint32_t seconds = 0U;
    rt_uint16_t year;
    rt_uint8_t month;

    if (datetime == RT_NULL || unix_time == RT_NULL)
    {
        return -RT_EINVAL;
    }

    if (datetime->year < 2000U || datetime->year > 2099U ||
        datetime->month < 1U || datetime->month > 12U ||
        datetime->day < 1U || datetime->day > app_rtc_month_days(datetime->year, datetime->month) ||
        datetime->hour > 23U || datetime->minute > 59U || datetime->second > 59U)
    {
        return -RT_EINVAL;
    }

    for (year = 1970U; year < datetime->year; year++)
    {
        seconds += app_rtc_is_leap_year(year) ? 31622400U : 31536000U;
    }

    for (month = 1U; month < datetime->month; month++)
    {
        seconds += (rt_uint32_t)app_rtc_month_days(datetime->year, month) * APP_RTC_SECONDS_PER_DAY;
    }

    seconds += (rt_uint32_t)(datetime->day - 1U) * APP_RTC_SECONDS_PER_DAY;
    seconds += (rt_uint32_t)datetime->hour * 3600U;
    seconds += (rt_uint32_t)datetime->minute * 60U;
    seconds += datetime->second;

    *unix_time = seconds;
    return RT_EOK;
}

void app_rtc_unix_to_datetime(rt_uint32_t unix_time, app_rtc_datetime_t *datetime)
{
    rt_uint32_t days;
    rt_uint32_t seconds_of_day;
    rt_uint16_t year;
    rt_uint8_t month;
    rt_uint16_t days_in_year;
    rt_uint8_t days_in_month;

    if (datetime == RT_NULL)
    {
        return;
    }

    days = unix_time / APP_RTC_SECONDS_PER_DAY;
    seconds_of_day = unix_time % APP_RTC_SECONDS_PER_DAY;

    year = 1970U;
    while (year < 2100U)
    {
        days_in_year = app_rtc_is_leap_year(year) ? 366U : 365U;
        if (days < days_in_year)
        {
            break;
        }
        days -= days_in_year;
        year++;
    }

    month = 1U;
    while (month <= 12U)
    {
        days_in_month = app_rtc_month_days(year, month);
        if (days < days_in_month)
        {
            break;
        }
        days -= days_in_month;
        month++;
    }

    datetime->year = year;
    datetime->month = month;
    datetime->day = (rt_uint8_t)(days + 1U);
    datetime->weekday = app_rtc_calculate_weekday(datetime->year, datetime->month, datetime->day);
    datetime->hour = (rt_uint8_t)(seconds_of_day / 3600U);
    seconds_of_day %= 3600U;
    datetime->minute = (rt_uint8_t)(seconds_of_day / 60U);
    datetime->second = (rt_uint8_t)(seconds_of_day % 60U);
}

rt_err_t app_rtc_init(void)
{
    app_rtc_datetime_t datetime;
    rt_err_t result;

    if (app_rtc_initialized != RT_FALSE)
    {
        return app_rtc_status;
    }

    app_rtc_mutex = rt_mutex_create("rtc_lock", RT_IPC_FLAG_PRIO);
    if (app_rtc_mutex == RT_NULL)
    {
        app_rtc_status = -RT_ERROR;
        return app_rtc_status;
    }

    ds1302_gpio_init();
    app_rtc_initialized = RT_TRUE;

    result = app_rtc_get_datetime(&datetime);
    if (result == RT_EOK)
    {
        app_rtc_valid = RT_TRUE;
    }
    else
    {
        app_rtc_valid = RT_FALSE;
    }

    app_rtc_status = RT_EOK;
    return RT_EOK;
}

rt_bool_t app_rtc_time_is_valid(void)
{
    return app_rtc_valid;
}

rt_err_t app_rtc_get_datetime(app_rtc_datetime_t *datetime)
{
    rt_err_t result;

    if (datetime == RT_NULL)
    {
        return -RT_EINVAL;
    }

    if (app_rtc_initialized == RT_FALSE)
    {
        return -RT_ERROR;
    }

    result = app_rtc_take();
    if (result != RT_EOK)
    {
        return result;
    }

    result = app_rtc_read_datetime_locked(datetime);
    app_rtc_valid = (result == RT_EOK) ? RT_TRUE : RT_FALSE;
    app_rtc_release();

    return result;
}

rt_err_t app_rtc_get_unix_time(rt_uint32_t *unix_time)
{
    app_rtc_datetime_t datetime;
    rt_err_t result;

    if (unix_time == RT_NULL)
    {
        return -RT_EINVAL;
    }

    result = app_rtc_get_datetime(&datetime);
    if (result != RT_EOK)
    {
        return result;
    }

    return app_rtc_datetime_to_unix(&datetime, unix_time);
}

rt_err_t app_rtc_set_datetime(const app_rtc_datetime_t *datetime)
{
    app_rtc_datetime_t fixed_datetime;
    rt_err_t result;

    if (datetime == RT_NULL)
    {
        return -RT_EINVAL;
    }

    fixed_datetime = *datetime;
    if (app_rtc_datetime_fields_are_valid(&fixed_datetime) == RT_FALSE)
    {
        return -RT_EINVAL;
    }

    if (fixed_datetime.weekday < 1U || fixed_datetime.weekday > 7U)
    {
        fixed_datetime.weekday = app_rtc_calculate_weekday(fixed_datetime.year,
                                                           fixed_datetime.month,
                                                           fixed_datetime.day);
    }

    if (app_rtc_datetime_is_valid(&fixed_datetime) == RT_FALSE)
    {
        return -RT_EINVAL;
    }

    if (app_rtc_initialized == RT_FALSE)
    {
        return -RT_ERROR;
    }

    result = app_rtc_take();
    if (result != RT_EOK)
    {
        return result;
    }

    ds1302_write_register(DS1302_CMD_WP_WRITE, 0x00U);
    ds1302_write_register(DS1302_CMD_SEC_WRITE, (rt_uint8_t)(ds1302_bin_to_bcd(fixed_datetime.second) | DS1302_CLOCK_HALT_BIT));
    ds1302_write_register(DS1302_CMD_MIN_WRITE, ds1302_bin_to_bcd(fixed_datetime.minute));
    ds1302_write_register(DS1302_CMD_HOUR_WRITE, ds1302_bin_to_bcd(fixed_datetime.hour));
    ds1302_write_register(DS1302_CMD_DATE_WRITE, ds1302_bin_to_bcd(fixed_datetime.day));
    ds1302_write_register(DS1302_CMD_MONTH_WRITE, ds1302_bin_to_bcd(fixed_datetime.month));
    ds1302_write_register(DS1302_CMD_DAY_WRITE, ds1302_bin_to_bcd(fixed_datetime.weekday));
    ds1302_write_register(DS1302_CMD_YEAR_WRITE, ds1302_bin_to_bcd((rt_uint8_t)(fixed_datetime.year - APP_RTC_YEAR_BASE)));
    ds1302_write_register(DS1302_CMD_SEC_WRITE, ds1302_bin_to_bcd(fixed_datetime.second));
    ds1302_write_register(DS1302_CMD_WP_WRITE, DS1302_WRITE_PROTECT_BIT);

    app_rtc_valid = RT_TRUE;
    app_rtc_release();

    return RT_EOK;
}

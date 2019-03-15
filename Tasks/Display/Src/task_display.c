/**
 * @brief		
 * @details		
 * @date		2017-01-09
 **/

/* Includes ------------------------------------------------------------------*/
#include "system.h"
#include "task_display.h"
#include "types_display.h"
#include "lcd.h"

#include "types_metering.h"
#include "types_protocol.h"
#include "types_keyboard.h"
#include "axdr.h"
#include "string.h"

/* Private typedef -----------------------------------------------------------*/
#define AMOUNT_LIST_MAX         ((uint8_t)(128))

/**
  * @brief  �������ʶ
  */
struct __disp_entry
{
    //����DLMS��˵��һ����ʾ�����һ�� __cosem_descriptor
    uint8_t                 descriptor[16];
};

/**
  * @brief  ��ʾ�б�����
  */
struct __disp_list
{
    //��Ч���������
	uint8_t					amount;
    
    //ÿ����ʾ�б����֧��128��������
	struct __disp_entry		entry[AMOUNT_LIST_MAX];
};

/**
  * @brief  ʱ�����
  */
struct __disp_time
{
    //�ϵ����ʱ������ʱ��
    uint8_t                 start;
    //ѭ���������л�ʱ��
    uint8_t                 scroll;
    //����ʱ��
    uint8_t                 backlight;
};

/**
  * @brief  ��������С����λ��
  */
struct __disp_dot
{
    //����
    uint32_t                power:4;
    //��ѹ
    uint32_t                voltage:4;
    //����
    uint32_t                current:4;
    //����
    uint32_t                energy:4;
    //����
    uint32_t                demand:4;
    //������������
    uint32_t                others:4;
};





/**
  * @brief  ��ʾ����EEPROM����
  */
struct __disp_param
{
    //ʱ�����
    struct __disp_time      time;
    //��������С����λ������
    struct __disp_dot       dots;
    //��ʾ�������б�
	struct __disp_list		list[DISP_CHANNEL_AMOUNT];
};

/**
  * @brief  ��ʾ�������в���
  */
struct __disp_runs
{
    //��������С����λ������
    struct __disp_dot       dots;
    //ʱ�����
    struct __disp_time      time;
    //ʱ�������
    struct __disp_time      counter;
    //��ǰ��ʾ�б�
    uint8_t                 channel;
    //֮ǰ����ʾ�б�
    uint8_t                 last_channel;
    //��ǰ��ʾ�б�����������
    uint8_t                 amount;
    //��ǰ��ʾ����������
    uint8_t                 index;
    //��ǰ������
    struct __disp_entry     entry;
    //��ѯ�붨ʱ
    uint16_t loop_record;
    //��ѯ����Ļ����ˢ��
    uint16_t loop_flush;
};

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static enum __task_status status = TASK_NOTINIT;
static struct __disp_runs disp_runs;

/* Private function prototypes -----------------------------------------------*/
static void clear_unit(void);
static uint32_t pow_uint(uint8_t base, uint8_t exp);
static enum __lcd_unit select_unit(struct __metering_identifier id);
static void select_data(uint32_t param, uint8_t *data);
static void flush_data(void);
static uint8_t display_list_show_index(uint8_t val);

/* Private functions ---------------------------------------------------------*/
static void clear_unit(void)
{
    lcd.label.off(LCD_LABEL_L1);
    lcd.label.off(LCD_LABEL_L2);
    lcd.label.off(LCD_LABEL_L3);
    lcd.label.off(LCD_LABEL_LN);
    lcd.label.off(LCD_LABEL_PF);
    lcd.label.off(LCD_LABEL_DATE);
    lcd.label.off(LCD_LABEL_TIM);
}

static enum __lcd_unit select_unit(struct __metering_identifier id)
{
	switch(id.phase)
	{
		case M_PHASE_A: //A��
		case M_PHASE_AB: //AB
			lcd.label.on(LCD_LABEL_L1, 0);
			break;
		case M_PHASE_B: //B��
		case M_PHASE_AC: //AC
			lcd.label.on(LCD_LABEL_L2, 0);
			break;
		case M_PHASE_C: //C��
		case M_PHASE_BC: //BC
			lcd.label.on(LCD_LABEL_L3, 0);
			break;
		case M_PHASE_N: //����
			lcd.label.on(LCD_LABEL_LN, 0);
			break;
	}
	
	switch(id.item)
	{
		case M_P_POWER: //�й����ʣ�mW��
			return(LCD_UNIT_W);
		case M_Q_POWER: //�޹����ʣ�mVar��
			return(LCD_UNIT_VAR);
		case M_S_POWER: //���ڹ��ʣ�mVA��
			return(LCD_UNIT_VA);
		case M_P_ENERGY: //�й����ܣ�mWh��
			return(LCD_UNIT_WH);
		case M_Q_ENERGY: //�޹����ܣ�mVarh��
			return(LCD_UNIT_VARH);
		case M_S_ENERGY: //���ڵ��ܣ�mVAh��
			return(LCD_UNIT_VARH);
		case M_P_CURRENT_DEMAND: //�й���ǰ������mW��
			return(LCD_UNIT_W);
		case M_Q_CURRENT_DEMAND: //�޹���ǰ������mVar��
			return(LCD_UNIT_VAR);
		case M_S_CURRENT_DEMAND: //���ڵ�ǰ������mVA��
			return(LCD_UNIT_VA);
		case M_P_MAX_DEMAND: //�й����������mW��
			return(LCD_UNIT_W);
		case M_Q_MAX_DEMAND: //�޹����������mVar��
			return(LCD_UNIT_VAR);
		case M_S_MAX_DEMAND: //�������������mVA��
			return(LCD_UNIT_VA);
		case M_VOLTAGE: //��ѹ��mV��
			return(LCD_UNIT_V);
		case M_CURRENT: //������mA��
			return(LCD_UNIT_A);
		case M_FREQUENCY: //Ƶ�ʣ�1/1000Hz��
			return(LCD_UNIT_HZ);
		case M_POWER_FACTOR: //����������1/1000��
			lcd.label.on(LCD_LABEL_PF, 0);
			return(LCD_UNIT_NONE);
	}
	
	return(LCD_UNIT_NONE);
}

static uint32_t pow_uint(uint8_t base, uint8_t exp)
{
    uint8_t count;
    uint32_t result = 1;
    
    for(count=0; count<exp; count++)
    {
        result *= base;
    }
    
    return(result);
}

static void select_data(uint32_t param, uint8_t *data)
{
    enum __axdr_type type;
    union __axdr_container container;
    struct __metering_identifier id;
    
    type = axdr.type.decode(data);
    
    if(AXDR_CONTAINED(type))
    {
        axdr.decode(data, 0, &container);
    }
    else
    {
        return;
    }
    
    M_UINT2ID(param, id);
    
    if(M_UINTISID(param))
    {
        if(M_UINTISVOLTAGE(param))
        {
            if(disp_runs.dots.voltage > 3)
            {
                container.i64_t *= pow_uint(10, (disp_runs.dots.voltage - 3));
            }
            else
            {
                container.i64_t /= pow_uint(10, (3 - disp_runs.dots.voltage));
            }
            lcd.window.show.dec(LCD_WINDOW_MAIN, (int32_t)container.i64_t, (enum __lcd_dot)disp_runs.dots.voltage, select_unit(id));
        }
        else if(M_UINTISCURRENT(param))
        {
            if(disp_runs.dots.current > 3)
            {
                container.i64_t *= pow_uint(10, (disp_runs.dots.current - 3));
            }
            else
            {
                container.i64_t /= pow_uint(10, (3 - disp_runs.dots.current));
            }
            lcd.window.show.dec(LCD_WINDOW_MAIN, (int32_t)container.i64_t, (enum __lcd_dot)disp_runs.dots.current, select_unit(id));
        }
        else if(M_UINTISPOWER(param))
        {
            if(disp_runs.dots.power > 3)
            {
                container.i64_t *= pow_uint(10, (disp_runs.dots.power - 3));
            }
            else
            {
                container.i64_t /= pow_uint(10, (3 - disp_runs.dots.power));
            }
            lcd.window.show.dec(LCD_WINDOW_MAIN, (int32_t)container.i64_t, (enum __lcd_dot)disp_runs.dots.power, select_unit(id));
        }
        else if(M_UINTISENERGY(param))
        {
            if(disp_runs.dots.energy > 3)
            {
                container.i64_t *= pow_uint(10, (disp_runs.dots.energy - 3));
            }
            else
            {
                container.i64_t /= pow_uint(10, (3 - disp_runs.dots.energy));
            }
            lcd.window.show.dec(LCD_WINDOW_MAIN, (int32_t)container.i64_t, (enum __lcd_dot)disp_runs.dots.energy, select_unit(id));
        }
        else if(M_UINTISDEMAND(param))
        {
            if(disp_runs.dots.demand > 3)
            {
                container.i64_t *= pow_uint(10, (disp_runs.dots.demand - 3));
            }
            else
            {
                container.i64_t /= pow_uint(10, (3 - disp_runs.dots.demand));
            }
            lcd.window.show.dec(LCD_WINDOW_MAIN, (int32_t)container.i64_t, (enum __lcd_dot)disp_runs.dots.demand, select_unit(id));
        }
        else if(M_UINTISDEMANDTIM(param))
        {
            lcd.window.show.date(LCD_WINDOW_MAIN, container.u64_t, LCD_DATE_MMDDhhmm);
        }
        else
        {
            lcd.window.show.dec(LCD_WINDOW_MAIN, (int32_t)container.i64_t, (enum __lcd_dot)disp_runs.dots.others, select_unit(id));
        }
    }
    else if(M_UINTISFMT(param))
    {
        switch(id.item)
        {
            case FMT_BIN: //�����Ƹ�ʽ
                lcd.window.show.bin(LCD_WINDOW_MAIN, container.u16_t, LCD_DOT_NONE, LCD_UNIT_NONE);
                break;
            case FMT_BCD: //...BCD��ʽ
            case FMT_HEX: //ʮ�����Ƹ�ʽ
                lcd.window.show.hex(LCD_WINDOW_MAIN, container.u32_t, LCD_DOT_NONE, LCD_UNIT_NONE);
                break;
            case FMT_DATE: //���ڸ�ʽ
                lcd.window.show.date(LCD_WINDOW_MAIN, container.u64_t, LCD_DATE_YYMMDD);
                break;
            case FMT_TIME: //ʱ���ʽ
                lcd.window.show.date(LCD_WINDOW_MAIN, container.u64_t, LCD_DATE_hhmmss);
                break;
            case FMT_DTIME: //����ʱ���ʽ
                lcd.window.show.date(LCD_WINDOW_MAIN, container.u64_t, LCD_DATE_MMDDhhmm);
                break;
            case FMT_STR: //�ַ�����ʽ
                lcd.window.show.msg(LCD_WINDOW_MAIN, (const char *)(data+2));
                break;
            default:
                lcd.window.show.dec(LCD_WINDOW_MAIN, container.i32_t, LCD_DOT_NONE, LCD_UNIT_NONE);
                break;
        }
    }
    else
    {
        lcd.window.show.dec(LCD_WINDOW_MAIN, container.i32_t, LCD_DOT_NONE, LCD_UNIT_NONE);
    }
}

static void flush_data(void)
{
    uint32_t param;
    uint8_t buff[32];
    struct __protocol *api_stream;
    
	if(disp_runs.index >= disp_runs.amount)
	{
		disp_runs.index = 0;
	}
    
    clear_unit();
    
    api_stream = api.query("task_protocol");
    
    if(!api_stream)
    {
        return;
    }
    
    if(!api_stream->read(disp_runs.entry.descriptor, buff, sizeof(buff), &param))
    {
        api.release();
        return;
    }
    
    api.release();
    
    select_data(param, buff);
    
    return;
}

static uint8_t display_change(uint8_t channel)
{
	uint32_t offset;
    
    if(channel >= DISP_CHANNEL_AMOUNT)
    {
        return(disp_runs.channel);
    }
    
    if(disp_runs.channel != DISP_CHANNEL_KEY)
    {
		disp_runs.last_channel = disp_runs.channel;
	}
	
    disp_runs.channel = channel;
    
    offset = STRUCT_OFFSET(struct __disp_param, list[disp_runs.channel].amount);
    disk.read("Display", offset, sizeof(disp_runs.amount), &disp_runs.amount);
    
    disp_runs.counter.scroll = 0;
    disp_runs.index = 0;
    display_list_show_index(0);
    
    return(disp_runs.channel);
}

static uint8_t display_channel(void)
{
    return(disp_runs.channel);
}

static uint8_t display_list_show_next(void)
{
    uint32_t offset;
    uint32_t param;
    uint8_t buff[32];
    struct __protocol *api_stream;
    
    //��ʾ��������һ
    disp_runs.index += 1;
    
	if(disp_runs.index >= disp_runs.amount)
	{
		disp_runs.index = 0;
	}
    
    //������������
	disp_runs.counter.scroll = 0;
    
    //����Ǽ��Բ��ҵ�ǰϵͳ״̬Ϊ��������״̬���򿪱���
    if((disp_runs.channel == DISP_CHANNEL_KEY) && (system_status() == SYSTEM_RUN))
    {
        lcd.backlight.open();
        disp_runs.counter.backlight = disp_runs.time.backlight;
    }
    
    //��յ�ǰ��L1 L2 L3 LN PF ��ʾ״̬
    clear_unit();
    
    //��ȡ��ʾ��������Ӧ���������ʶ
    offset = STRUCT_OFFSET(struct __disp_param, list[disp_runs.channel].entry[disp_runs.index]);
    disk.read("Display", offset, sizeof(disp_runs.entry), &disp_runs.entry);
    
    api_stream = api.query("task_protocol");
    if(!api_stream)
    {
        return(disp_runs.index);
    }
    
    if(!api_stream->read(disp_runs.entry.descriptor, buff, sizeof(buff), &param))
    {
        api.release();
        return(disp_runs.index);
    }
    
    api.release();
    
    //���ݻ�ȡ�������ݺͲ�����ˢ����ʾ
    select_data(param, buff);
    
    return(disp_runs.index);
}

static uint8_t display_list_show_last(void)
{
    uint32_t offset;
    uint32_t param;
    uint8_t buff[32];
    struct __protocol *api_stream;
    
	if(disp_runs.index)
	{
        disp_runs.index -= 1;
	}
    else
    {
        disp_runs.index = disp_runs.amount - 1;
    }
    
	disp_runs.counter.scroll = 0;
    
    if((disp_runs.channel == DISP_CHANNEL_KEY) && (system_status() == SYSTEM_RUN))
    {
        lcd.backlight.open();
        disp_runs.counter.backlight = disp_runs.time.backlight;
    }
    
    clear_unit();
    
    offset = STRUCT_OFFSET(struct __disp_param, list[disp_runs.channel].entry[disp_runs.index]);
    disk.read("Display", offset, sizeof(disp_runs.entry), &disp_runs.entry);
    
    api_stream = api.query("task_protocol");
    if(!api_stream)
    {
        return(disp_runs.index);
    }
	
    if(!api_stream->read(disp_runs.entry.descriptor, buff, sizeof(buff), &param))
    {
        api.release();
        return(disp_runs.index);
    }
    
    api.release();
    
    select_data(param, buff);
    
    return(disp_runs.index);
}

static uint8_t display_list_show_index(uint8_t val)
{
    uint32_t offset;
    uint32_t param;
    uint8_t buff[32];
    struct __protocol *api_stream;
    
	if(val >= disp_runs.amount)
	{
        return(disp_runs.index);
	}
    
    disp_runs.index = val;
    
	disp_runs.counter.scroll = 0;
    
    if((disp_runs.channel == DISP_CHANNEL_KEY) && (system_status() == SYSTEM_RUN))
    {
        lcd.backlight.open();
        disp_runs.counter.backlight = disp_runs.time.backlight;
    }
    
    clear_unit();
    
    offset = STRUCT_OFFSET(struct __disp_param, list[disp_runs.channel].entry[disp_runs.index]);
    disk.read("Display", offset, sizeof(disp_runs.entry), &disp_runs.entry);
    
    api_stream = api.query("task_protocol");
    if(!api_stream)
    {
        return(disp_runs.index);
    }
    
    if(!api_stream->read(disp_runs.entry.descriptor, buff, sizeof(buff), &param))
    {
        api.release();
        return(disp_runs.index);
    }
    
    api.release();
    
    select_data(param, buff);
    
    return(disp_runs.index);
}

static uint8_t display_list_current(void)
{
    return(disp_runs.index);
}

static uint8_t display_config_start_get(void)
{
    return(disp_runs.time.start);
}

static uint8_t display_config_start_set(uint8_t second)
{
	uint32_t offset;
    
    disp_runs.time.start = second;
	offset = STRUCT_OFFSET(struct __disp_param, time.start);
	disk.write("Display", offset, sizeof(disp_runs.time.start), &disp_runs.time.start);
    
    return(disp_runs.time.start);
}

static uint8_t display_config_scroll_get(void)
{
    return(disp_runs.time.scroll);
}

static uint8_t display_config_scroll_set(uint8_t second)
{
	uint32_t offset;
    
    disp_runs.time.scroll = second;
	offset = STRUCT_OFFSET(struct __disp_param, time.scroll);
	disk.write("Display", offset, sizeof(disp_runs.time.scroll), &disp_runs.time.scroll);
    
    return(disp_runs.time.scroll);
}

static uint8_t display_config_backlight_get(void)
{
    return(disp_runs.time.backlight);
}

static uint8_t display_config_backlight_set(uint8_t second)
{
	uint32_t offset;
    
    disp_runs.time.backlight = second;
	offset = STRUCT_OFFSET(struct __disp_param, time.backlight);
	disk.write("Display", offset, sizeof(disp_runs.time.backlight), &disp_runs.time.backlight);
    
    return(disp_runs.time.backlight);
}

static uint8_t display_config_power_dot_get(void)
{
    return((uint8_t)disp_runs.dots.power);
}

static uint8_t display_config_power_dot_set(uint8_t dot)
{
	uint32_t offset;
    struct __disp_dot dots;
    
	offset = STRUCT_OFFSET(struct __disp_param, dots);
    disk.read("Display", offset, sizeof(disp_runs.dots), &dots);
    disp_runs.dots.power = dot;
	disk.write("Display", offset, sizeof(disp_runs.dots), &dots);
    
    return((uint8_t)disp_runs.dots.power);
}


static uint8_t display_config_voltage_dot_get(void)
{
    return((uint8_t)disp_runs.dots.voltage);
}

static uint8_t display_config_voltage_dot_set(uint8_t dot)
{
	uint32_t offset;
    struct __disp_dot dots;
    
	offset = STRUCT_OFFSET(struct __disp_param, dots);
    disk.read("Display", offset, sizeof(disp_runs.dots), &dots);
    disp_runs.dots.voltage = dot;
	disk.write("Display", offset, sizeof(disp_runs.dots), &dots);
    
    return((uint8_t)disp_runs.dots.voltage);
}

static uint8_t display_config_current_dot_get(void)
{
    return((uint8_t)disp_runs.dots.current);
}

static uint8_t display_config_current_dot_set(uint8_t dot)
{
	uint32_t offset;
    struct __disp_dot dots;
    
	offset = STRUCT_OFFSET(struct __disp_param, dots);
    disk.read("Display", offset, sizeof(disp_runs.dots), &dots);
    disp_runs.dots.current = dot;
	disk.write("Display", offset, sizeof(disp_runs.dots), &dots);
    
    return((uint8_t)disp_runs.dots.current);
}

static uint8_t display_config_energy_dot_get(void)
{
    return((uint8_t)disp_runs.dots.energy);
}

static uint8_t display_config_energy_dot_set(uint8_t dot)
{
	uint32_t offset;
    struct __disp_dot dots;
    
	offset = STRUCT_OFFSET(struct __disp_param, dots);
    disk.read("Display", offset, sizeof(disp_runs.dots), &dots);
    disp_runs.dots.energy = dot;
	disk.write("Display", offset, sizeof(disp_runs.dots), &dots);
    
    return((uint8_t)disp_runs.dots.energy);
}

static uint8_t display_config_demand_dot_get(void)
{
    return((uint8_t)disp_runs.dots.demand);
}

static uint8_t display_config_demand_dot_set(uint8_t dot)
{
	uint32_t offset;
    struct __disp_dot dots;
    
	offset = STRUCT_OFFSET(struct __disp_param, dots);
    disk.read("Display", offset, sizeof(disp_runs.dots), &dots);
    disp_runs.dots.demand = dot;
	disk.write("Display", offset, sizeof(disp_runs.dots), &dots);
    
    return((uint8_t)disp_runs.dots.demand);
}

static uint8_t display_config_others_dot_get(void)
{
    return((uint8_t)disp_runs.dots.others);
}

static uint8_t display_config_others_dot_set(uint8_t dot)
{
	uint32_t offset;
    struct __disp_dot dots;
    
	offset = STRUCT_OFFSET(struct __disp_param, dots);
    disk.read("Display", offset, sizeof(disp_runs.dots), &dots);
    disp_runs.dots.others = dot;
	disk.write("Display", offset, sizeof(disp_runs.dots), &dots);
    
    return((uint8_t)disp_runs.dots.others);
}

static uint16_t display_config_list_write(uint8_t channel, void *id)
{
	uint32_t offset;
    uint8_t amount = 0;
    
    if(channel >= DISP_CHANNEL_AMOUNT)
    {
        return(0);
    }
    
    if(!id)
    {
        return(0);
    }
    
	offset = STRUCT_OFFSET(struct __disp_param, list[channel].amount);
	disk.read("Display", offset, sizeof(amount), &amount);
    
    if(amount >= AMOUNT_LIST_MAX)
    {
        return(16);
    }
    
	offset = STRUCT_OFFSET(struct __disp_param, list[channel].entry[3]);
	disk.write("Display", offset, 16, id);
    
    amount += 1;
    
	offset = STRUCT_OFFSET(struct __disp_param, list[channel].amount);
	disk.write("Display", offset, sizeof(amount), &amount);
    
    if(channel == disp_runs.channel)
    {
        disp_runs.amount = amount;
    }
    
    return(16);
}

static uint16_t display_config_list_read(uint8_t channel, uint16_t index, void *id)
{
	uint32_t offset;
    uint8_t amount = 0;
    
    if(channel >= DISP_CHANNEL_AMOUNT)
    {
        return(0);
    }
    
    if(!id)
    {
        return(0);
    }
    
	offset = STRUCT_OFFSET(struct __disp_param, list[channel].amount);
	disk.read("Display", offset, sizeof(amount), &amount);
    
    if(index >= amount)
    {
        return(0);
    }
    
	offset = STRUCT_OFFSET(struct __disp_param, list[channel].entry[index]);
	disk.read("Display", offset, 16, id);
    
    return(16);
}

static uint8_t display_config_list_amount(uint8_t channel)
{
	uint32_t offset;
    uint8_t amount = 0;
    
    if(channel >= DISP_CHANNEL_AMOUNT)
    {
        return(0);
    }
    
	offset = STRUCT_OFFSET(struct __disp_param, list[channel].amount);
	disk.read("Display", offset, sizeof(amount), &amount);
    
    return(amount);
}

static uint8_t display_config_list_clean(uint8_t channel)
{
	uint32_t offset;
    uint8_t amount = 0;
    
    if(channel >= DISP_CHANNEL_AMOUNT)
    {
        return(0);
    }
    
	offset = STRUCT_OFFSET(struct __disp_param, list[channel].amount);
	disk.write("Display", offset, sizeof(amount), &amount);
    
    if(channel == disp_runs.channel)
    {
        disp_runs.amount = 0;
    }
    
    return(0);
}


static const struct __display display = 
{
    .change                     = display_change,
    .channel                    = display_channel,
    
    .list                       =
    {
        .show                   =
        {
            .next               = display_list_show_next,
            .last               = display_list_show_last,
            .index              = display_list_show_index,
        },
        
        .current                = display_list_current,
    },
    
    .config                     =
    {
        .time                   =
        {
            .start              = 
            {
                .get            = display_config_start_get,
                .set            = display_config_start_set,
            },
            
            .scroll             = 
            {
                .get            = display_config_scroll_get,
                .set            = display_config_scroll_set,
            },
            
            .backlight          = 
            {
                .get            = display_config_backlight_get,
                .set            = display_config_backlight_set,
            },
        },
        
        .dot                    =
        {
            .power              = 
            {
                .get            = display_config_power_dot_get,
                .set            = display_config_power_dot_set,
            },
            
            .voltage            = 
            {
                .get            = display_config_voltage_dot_get,
                .set            = display_config_voltage_dot_set,
            },
            
            .current            = 
            {
                .get            = display_config_current_dot_get,
                .set            = display_config_current_dot_set,
            },
            
            .energy             = 
            {
                .get            = display_config_energy_dot_get,
                .set            = display_config_energy_dot_set,
            },
            
            .demand             = 
            {
                .get            = display_config_demand_dot_get,
                .set            = display_config_demand_dot_set,
            },
            
            .others             = 
            {
                .get            = display_config_others_dot_get,
                .set            = display_config_others_dot_set,
            },
        },
        
        .list                   = 
        {
            .write              = display_config_list_write,
            .read               = display_config_list_read,
            .amount             = display_config_list_amount,
            .clean              = display_config_list_clean,
        }
    },
};




/**
  * @brief  
  */
static void display_init(void)
{
	uint32_t offset;
	
    if((system_status() == SYSTEM_RUN) || (system_status() == SYSTEM_WAKEUP))
    {
        heap.set(&disp_runs, 0, sizeof(disp_runs));
        
        disp_runs.channel = DISP_CHANNEL_AUTO;
        disp_runs.last_channel = DISP_CHANNEL_AUTO;
        
        offset = STRUCT_OFFSET(struct __disp_param, dots);
        disk.read("Display", offset, sizeof(disp_runs.dots), &disp_runs.dots);
        
		offset = STRUCT_OFFSET(struct __disp_param, time);
		disk.read("Display", offset, sizeof(disp_runs.time), &disp_runs.time);
		
		offset = STRUCT_OFFSET(struct __disp_param, list[disp_runs.channel].amount);
		disk.read("Display", offset, sizeof(disp_runs.amount), &disp_runs.amount);
		
		offset = STRUCT_OFFSET(struct __disp_param, list[disp_runs.channel].entry[disp_runs.index]);
		disk.read("Display", offset, sizeof(disp_runs.entry), &disp_runs.entry);
        
        if(system_status() == SYSTEM_RUN)
        {
            lcd.control.init(DEVICE_NORMAL);
            lcd.backlight.open();
            disp_runs.counter.backlight = disp_runs.time.backlight;
        }
        else
        {
            lcd.control.init(DEVICE_LOWPOWER);
        }
        
        status = TASK_INIT;
    }
}

/**
  * @brief  
  */
static void display_loop(void)
{
    if((system_status() == SYSTEM_RUN) || (system_status() == SYSTEM_WAKEUP))
    {
        status = TASK_RUN;
        lcd.runner(KERNEL_PERIOD);
        
        disp_runs.loop_record += KERNEL_PERIOD;
        
        //ȫ�����֮��ſ�ʼ100ms�Ķ�ʱˢ��
        if(disp_runs.counter.start >= disp_runs.time.start)
        {
            disp_runs.loop_flush += KERNEL_PERIOD;
        }
        
        //����״̬�£�ֱ�ӹرձ���
        if(lcd.backlight.status() && (system_status() == SYSTEM_WAKEUP))
        {
            disp_runs.counter.backlight = 0;
            
            lcd.backlight.close();
        }
        
        
        //����ʱ
		if(disp_runs.loop_record > 999)
		{
			disp_runs.loop_record = 0;
            disp_runs.loop_flush = 0;
            
            //������ʱ����
            if(disp_runs.counter.backlight)
            {
                disp_runs.counter.backlight -= 1;
                
                if(disp_runs.counter.backlight == 0)
                {
                    if(lcd.backlight.status())
                    {
                        lcd.backlight.close();
                    }
                }
            }
            
            //�Ǽ����£��Զ�����
            if((disp_runs.channel == DISP_CHANNEL_AUTO) || (disp_runs.channel == DISP_CHANNEL_DEBUG))
            {
                //�����ǰ���ϵ�ȫ��״̬���������´���
                if(disp_runs.counter.start < disp_runs.time.start)
                {
                    disp_runs.counter.start += 1;
                    
                    if(disp_runs.counter.start >= disp_runs.time.start)
                    {
                        display.list.show.index(disp_runs.index);
                    }
                    
                    return;
                }
                
                //����ʱ���ʱ
                disp_runs.counter.scroll += 1;
                
                //����ʱ���ʱ�������õĹ���ʱ�䣬��ʼ����
                if(disp_runs.counter.scroll >= disp_runs.time.scroll)
                {
                    display.list.show.next();
                }
            }
            //�����£�����ʱ�䵽��֮���л�֮ǰ����ʾ�б�
            else if(disp_runs.channel == DISP_CHANNEL_KEY)
            {
                disp_runs.counter.scroll += 1;
                
                if(disp_runs.counter.scroll >= disp_runs.time.scroll)
                {
                	display.change(disp_runs.last_channel);
                }
                
                //�л������Ժ�ʧ���ϵ�ȫ��
                disp_runs.counter.start = disp_runs.time.start + 1;
            }
		}
        
        //100msˢ�������ڵ���ʾ����
        if(disp_runs.loop_flush > 99)
        {
            disp_runs.loop_flush = 0;
            flush_data();
        }
    }
}

/**
  * @brief  
  */
static void display_exit(void)
{
    lcd.control.suspend();
    status = TASK_SUSPEND;
}

/**
  * @brief  
  */
static void display_reset(void)
{
    lcd.control.suspend();
    status = TASK_NOTINIT;
    
//...��������ʹ��
//��ʼ��EEPROM����
	uint32_t offset;
	uint8_t val;
    uint8_t descriptor[16];
    struct __disp_dot dots;
    
    descriptor[0] = 0;
    descriptor[1] = 3;
    
    descriptor[2] = 1;
    descriptor[3] = 0;
    descriptor[4] = 0;
    descriptor[5] = 7;
    descriptor[6] = 0;
    descriptor[7] = 255;
    
    descriptor[8] = 2;
    
    descriptor[9] = 0;
    descriptor[10] = 0;
	
	dots.power = 3;
    dots.voltage = 1;
    dots.current = 3;
    dots.energy = 3;
    dots.demand = 4;
    
	offset = STRUCT_OFFSET(struct __disp_param, dots);
	disk.write("Display", offset, sizeof(dots), &dots);
    
	val = 6;
	offset = STRUCT_OFFSET(struct __disp_param, time.scroll);
	disk.write("Display", offset, sizeof(val), &val);
    
	val = 8;
	offset = STRUCT_OFFSET(struct __disp_param, time.backlight);
	disk.write("Display", offset, sizeof(val), &val);
    
    val = 3;
	offset = STRUCT_OFFSET(struct __disp_param, time.start);
	disk.write("Display", offset, sizeof(val), &val);
	
	val = 6;
	offset = STRUCT_OFFSET(struct __disp_param, list[DISP_CHANNEL_AUTO].amount);
	disk.write("Display", offset, sizeof(val), &val);
    offset = STRUCT_OFFSET(struct __disp_param, list[DISP_CHANNEL_KEY].amount);
	disk.write("Display", offset, sizeof(val), &val);
	
    descriptor[4] = 32;
	offset = STRUCT_OFFSET(struct __disp_param, list[DISP_CHANNEL_AUTO].entry[0]);
	disk.write("Display", offset, sizeof(descriptor), &descriptor);
    offset = STRUCT_OFFSET(struct __disp_param, list[DISP_CHANNEL_KEY].entry[0]);
	disk.write("Display", offset, sizeof(descriptor), &descriptor);
	
    descriptor[4] = 52;
	offset = STRUCT_OFFSET(struct __disp_param, list[DISP_CHANNEL_AUTO].entry[1]);
	disk.write("Display", offset, sizeof(descriptor), &descriptor);
    offset = STRUCT_OFFSET(struct __disp_param, list[DISP_CHANNEL_KEY].entry[1]);
	disk.write("Display", offset, sizeof(descriptor), &descriptor);
	
    descriptor[4] = 72;
	offset = STRUCT_OFFSET(struct __disp_param, list[DISP_CHANNEL_AUTO].entry[2]);
	disk.write("Display", offset, sizeof(descriptor), &descriptor);
    offset = STRUCT_OFFSET(struct __disp_param, list[DISP_CHANNEL_KEY].entry[2]);
	disk.write("Display", offset, sizeof(descriptor), &descriptor);
	
    descriptor[4] = 31;
	offset = STRUCT_OFFSET(struct __disp_param, list[DISP_CHANNEL_AUTO].entry[3]);
	disk.write("Display", offset, sizeof(descriptor), &descriptor);
    offset = STRUCT_OFFSET(struct __disp_param, list[DISP_CHANNEL_KEY].entry[3]);
	disk.write("Display", offset, sizeof(descriptor), &descriptor);
	
    descriptor[4] = 51;
	offset = STRUCT_OFFSET(struct __disp_param, list[DISP_CHANNEL_AUTO].entry[4]);
	disk.write("Display", offset, sizeof(descriptor), &descriptor);
    offset = STRUCT_OFFSET(struct __disp_param, list[DISP_CHANNEL_KEY].entry[4]);
	disk.write("Display", offset, sizeof(descriptor), &descriptor);
	
    descriptor[4] = 71;
	offset = STRUCT_OFFSET(struct __disp_param, list[DISP_CHANNEL_AUTO].entry[5]);
	disk.write("Display", offset, sizeof(descriptor), &descriptor);
    offset = STRUCT_OFFSET(struct __disp_param, list[DISP_CHANNEL_KEY].entry[5]);
	disk.write("Display", offset, sizeof(descriptor), &descriptor);

//...��������ʹ��
}

/**
  * @brief  
  */
static enum __task_status display_status(void)
{
    return(status);
}


/**
  * @brief  
  */
const struct __task_sched task_display = 
{
    .name               = "task_display",
    .init               = display_init,
    .loop               = display_loop,
    .exit               = display_exit,
    .reset              = display_reset,
    .status             = display_status,
    .api                = (void *)&display,
};




static void keyboard_callback(void *args)
{
    struct __keyboard_event *event = (struct __keyboard_event *)args;
    
    if(status != TASK_RUN)
    {
        return;
    }
    
    //�̰��ϼ������¼�����ǰ��ʾ�б����Ǽ����б������л��������б�
    if(display_channel() != DISP_CHANNEL_KEY)
    {
        display_change(DISP_CHANNEL_KEY);
        return;
    }
    
    //�̰��ϼ�����ʾ��һ��
    if(event->id == KEY_ID_UP)
    {
        display_list_show_last();
        return;
    }
    
    //�̰��¼�����ʾ��һ��
    if(event->id == KEY_ID_DOWN)
    {
        display_list_show_next();
        return;
    }
}

CALLBACK_DEF(display_key_changed, keyboard_callback);
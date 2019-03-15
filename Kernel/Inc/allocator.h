/**
 * @brief		
 * @details		
 * @date		2018-08-30
 **/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ALLOCATOR_H__
#define __ALLOCATOR_H__

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"

/* Exported types ------------------------------------------------------------*/
/**
  * @brief  �洢������
  */
enum __disk_aria
{
    DISK_SAFE = 0x03,//˫��������
    DISK_FREQ = 0x0c,//����������
    DISK_WIDE = 0x30,//����������
};

/**
  * @brief  �洢���ӿ�
  */
struct __disk
{
    uint32_t                    (*alloc)(enum __disk_aria aria, const char *name, uint32_t size);
    void                        (*free)(const char *name);
    uint32_t                    (*seek)(const char *name);
    uint32_t                    (*read)(const char *name, uint32_t offset, uint32_t count, void *buff);
    uint32_t                    (*write)(const char *name, uint32_t offset, uint32_t count, const void *buff);
    uint32_t                    (*copy)(const char *dst, const char *src, uint32_t count);
    uint32_t                    (*set)(const char *name, uint8_t ch, uint32_t count);
};

/**
  * @brief  �ѽӿ�
  */
struct __heap_info
{
    uint32_t                        (*size)(void);
    uint32_t                        (*available)(void);
};

/**
  * @brief  �ѽӿ�
  */
struct __heap
{
    void                            *(*salloc)(uint32_t size);
    void                            *(*scalloc)(uint32_t n, uint32_t size);
    void                            *(*dalloc)(uint32_t size);
    void                            *(*dcalloc)(uint32_t n, uint32_t size);
    void                            (*free)(void *address);
    uint32_t                        (*copy)(void *dst, const void *src, uint32_t count);
    uint32_t                        (*set)(void *address, uint8_t ch, uint32_t count);
    struct __heap_info              szone;
    struct __heap_info              dzone;
};

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
#if defined ( _WIN64 ) || defined ( __amd64__ ) || defined ( __x86_64 )

#define STRUCT_OFFSET(name, member)         ((uint64_t)(&((name *)0)->member))
#define STRUCT_SIZE(name, member)           ((uint64_t)(sizeof((name *)0)->member))

#else

#define STRUCT_OFFSET(name, member)         ((uint32_t)(&((name *)0)->member))
#define STRUCT_SIZE(name, member)           ((uint32_t)(sizeof((name *)0)->member))

#endif

#define HEAP_FORWARD(p, c)                  heap.set((void *)(*p), c, 1); (uint8_t *)(*p)++

/* Exported function prototypes ----------------------------------------------*/
extern struct __heap heap;
extern struct __disk disk;

#endif /* __ALLOCATOR_H__ */
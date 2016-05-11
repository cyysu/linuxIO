#ifndef _LINUX_VIRTIO_H
#define _LINUX_VIRTIO_H
/* Everything a virtio driver needs to work with any particular virtio
 * implementation. */
#include <linux/types.h>
#include <linux/scatterlist.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/mod_devicetable.h>
#include <linux/gfp.h>
#include <linux/vringh.h>

/**
 * virtqueue - a queue to register buffers for sending or receiving.
 * @list: the chain of virtqueues for this device
 * @callback: the function to call when buffers are consumed (can be NULL).
 * @name: the name of this virtqueue (mainly for debugging)
 * @vdev: the virtio device this queue was created for.
 * @priv: a pointer for the virtqueue implementation to use.
 * @index: the zero-based ordinal number for this queue.
 * @num_free: number of elements we expect to be able to fit.
 *
 * A note on @num_free: with indirect buffers, each buffer needs one
 * element in the queue, otherwise a buffer will need one element per
 * sg element.
 */
struct virtqueue {  //一个队列注册buffers用于 send or receive
	struct list_head list;       //这个设备的virtqueues        
	void (*callback)(struct virtqueue *vq); //当 buffers are consumed 的时候调用的函数
	const char *name;               //这个virtqueue 的名称，主要用于调试
	struct virtio_device *vdev;    //对应的virtio 设备
	unsigned int index;            //这个队列的index
	unsigned int num_free;         //填充的数量,对于间接buffers，每一个buffer需要在队列中有一个对应的element，否则在每个sg中对应一个element
	void *priv;                    //
};

int virtqueue_add_outbuf(struct virtqueue *vq,
			 struct scatterlist sg[], unsigned int num,
			 void *data,
			 gfp_t gfp);   //用于向queue中添加一个新的BUFF，参数data是一个非空的令牌，用于识别buffer

int virtqueue_add_inbuf(struct virtqueue *vq,
			struct scatterlist sg[], unsigned int num,
			void *data,
			gfp_t gfp);

int virtqueue_add_sgs(struct virtqueue *vq,
		      struct scatterlist *sgs[],
		      unsigned int out_sgs,
		      unsigned int in_sgs,
		      void *data,
		      gfp_t gfp);

bool virtqueue_kick(struct virtqueue *vq);  //guest通知host单个或者多个buffer已经被添加到queue中，调用virtqueue_notify向virtio_pci_queue_notify寄存器写入queue index来通知host

bool virtqueue_kick_prepare(struct virtqueue *vq);

bool virtqueue_notify(struct virtqueue *vq);

void *virtqueue_get_buf(struct virtqueue *vq, unsigned int *len);//返回使用过的buffer，len为写入到buffer中的数据长度，获取数据，释放buffer，更新vring描述符中的index

void virtqueue_disable_cb(struct virtqueue *vq);//告诉guest不再需要知道一个buffer是否已经使用，关闭device的中断

bool virtqueue_enable_cb(struct virtqueue *vq);

unsigned virtqueue_enable_cb_prepare(struct virtqueue *vq);

bool virtqueue_poll(struct virtqueue *vq, unsigned);

bool virtqueue_enable_cb_delayed(struct virtqueue *vq);

void *virtqueue_detach_unused_buf(struct virtqueue *vq);

unsigned int virtqueue_get_vring_size(struct virtqueue *vq);

bool virtqueue_is_broken(struct virtqueue *vq);

void *virtqueue_get_avail(struct virtqueue *vq);
void *virtqueue_get_used(struct virtqueue *vq);

/**
 * virtio_device - representation of a device using virtio
 * @index: unique position on the virtio bus
 * @failed: saved value for VIRTIO_CONFIG_S_FAILED bit (for restore)
 * @config_enabled: configuration change reporting enabled
 * @config_change_pending: configuration change reported while disabled
 * @config_lock: protects configuration change reporting
 * @dev: underlying device.
 * @id: the device type identification (used to match it with a driver).
 * @config: the configuration ops for this device.
 * @vringh_config: configuration ops for host vrings.
 * @vqs: the list of virtqueues for this device.
 * @features: the features supported by both driver and device.
 * @priv: private pointer for the driver's use.
 */
struct virtio_device {
	int index;                    //virtio bus上的position
	bool failed;                  //保存virtio_config_s_failed位的值
	bool config_enabled;          //允许改变配置
	bool config_change_pending;   //不准改变配置
	spinlock_t config_lock;       //配置锁
	struct device dev;            //设备
	struct virtio_device_id id;   //设备类型标志符
	const struct virtio_config_ops *config;  // 该设备的configuration ops
	const struct vringh_config_ops *vringh_config; //host vrings的配置操作
	struct list_head vqs;                          //该设备的virtqueuesqueue
	u64 features;                                 //驱动和设备同时支持的特性
	void *priv;                             //驱动使用的私有指针 指向virtio_blk
};

static inline struct virtio_device *dev_to_virtio(struct device *_dev)
{
	return container_of(_dev, struct virtio_device, dev);
}

int register_virtio_device(struct virtio_device *dev);
void unregister_virtio_device(struct virtio_device *dev);

void virtio_break_device(struct virtio_device *dev);

void virtio_config_changed(struct virtio_device *dev);
#ifdef CONFIG_PM_SLEEP
int virtio_device_freeze(struct virtio_device *dev);
int virtio_device_restore(struct virtio_device *dev);
#endif

/**
 * virtio_driver - operations for a virtio I/O driver
 * @driver: underlying device driver (populate name and owner).
 * @id_table: the ids serviced by this driver.
 * @feature_table: an array of feature numbers supported by this driver.
 * @feature_table_size: number of entries in the feature table array.
 * @feature_table_legacy: same as feature_table but when working in legacy mode.
 * @feature_table_size_legacy: number of entries in feature table legacy array.
 * @probe: the function to call when a device is found.  Returns 0 or -errno.
 * @remove: the function to call when a device is removed.
 * @config_changed: optional function to call when the device configuration
 *    changes; may be called in interrupt context.
 */
struct virtio_driver {
	struct device_driver driver;                //设备驱动
	const struct virtio_device_id *id_table;    //驱动服务的ids
	const unsigned int *feature_table;          //这个驱动支持一组feature numbers
	unsigned int feature_table_size;            //在feature table array中的入口数量
	const unsigned int *feature_table_legacy;   
	unsigned int feature_table_size_legacy;
	int (*probe)(struct virtio_device *dev);   //当设备被发现时调用的函数
	void (*scan)(struct virtio_device *dev);   
	void (*remove)(struct virtio_device *dev); //当设备被remove时调用的函数
	void (*config_changed)(struct virtio_device *dev); //当设备配置被change时可选的函数调用
#ifdef CONFIG_PM
	int (*freeze)(struct virtio_device *dev);
	int (*restore)(struct virtio_device *dev);
#endif
};

static inline struct virtio_driver *drv_to_virtio(struct device_driver *drv)
{
	return container_of(drv, struct virtio_driver, driver);
}

int register_virtio_driver(struct virtio_driver *drv);
void unregister_virtio_driver(struct virtio_driver *drv);

/* module_virtio_driver() - Helper macro for drivers that don't do
 * anything special in module init/exit.  This eliminates a lot of
 * boilerplate.  Each module may only use this macro once, and
 * calling it replaces module_init() and module_exit()
 */
#define module_virtio_driver(__virtio_driver) \
	module_driver(__virtio_driver, register_virtio_driver, \
			unregister_virtio_driver)
#endif /* _LINUX_VIRTIO_H */

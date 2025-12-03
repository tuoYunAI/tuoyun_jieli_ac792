#ifndef HCI_EVENT_H
#define HCI_EVENT_H

#include "app_config.h"
#include "btctrler/btcontroller_mode.h"

///搜索完整结束的事件
#define HCI_EVENT_INQUIRY_COMPLETE                            0x01
///连接完成的事件
#define HCI_EVENT_CONNECTION_COMPLETE                         0x03
///断开的事件,会有一些错误码区分不同情况的断开
#define HCI_EVENT_DISCONNECTION_COMPLETE                      0x05
///连接中请求pin code的事件,给这个事件上来目前是做了一个确认的操作
#define HCI_EVENT_PIN_CODE_REQUEST                            0x16
///连接设备发起了简易配对加密的连接
#define HCI_EVENT_IO_CAPABILITY_REQUEST                       0x31
///这个事件上来目前是做了一个连接确认的操作
#define HCI_EVENT_USER_CONFIRMATION_REQUEST                   0x33
///这个事件是需要输入6个数字的连接消息，一般在键盘应用才会有
#define HCI_EVENT_USER_PASSKEY_REQUEST                        0x34
///<可用于显示输入passkey位置 value 0:start  1:enrer  2:earse   3:clear  4:complete
#define HCI_EVENT_USER_PRESSKEY_NOTIFICATION			      0x3B
///杰理自定义的事件，上电回连的时候读不到VM的地址。
#define HCI_EVENT_VENDOR_NO_RECONN_ADDR                       0xF8
///杰理自定义的事件，有测试盒连接的时候会有这个事件
#define HCI_EVENT_VENDOR_REMOTE_TEST                          0xFE
///杰理自定义的事件，可以认为是断开之后协议栈资源释放完成的事件
#define BTSTACK_EVENT_HCI_CONNECTIONS_DELETE                  0x6D


#define ERROR_CODE_SUCCESS                                    0x00
///<回连超时退出消息
#define ERROR_CODE_PAGE_TIMEOUT                               0x04
///<连接过程中linkkey错误
#define ERROR_CODE_AUTHENTICATION_FAILURE                     0x05
///<连接过程中linkkey丢失，手机删除了linkkey，回连就会出现一次
#define ERROR_CODE_PIN_OR_KEY_MISSING                         0x06
///<连接超时，一般拿远就会断开有这个消息
#define ERROR_CODE_CONNECTION_TIMEOUT                         0x08
///<连接个数超出了资源允许
#define ERROR_CODE_SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED  0x0A
///<回连的时候发现设备还没释放我们这个地址，一般直接断电开机回连会出现
#define ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS                      0x0B
///<需要回连的设备资源不够。有些手机连了一个设备之后就会用这个拒绝。或者其它的资源原因
#define ERROR_CODE_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES       0x0D
///<pincode模式连接时手机输错pin后返回的错误码
#define ERROR_CODE_CONNECTION_REJECTED_DUE_TO_SECURITY_REASONS        0x0E
///<有些可能只允许指定地址连接的，可能会用这个去拒绝连接。或者我们设备的地址全0或者有问题
#define ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR    0x0F
///<连接的时间太长了，手机要断开了。这种容易复现可以联系我们分析
#define ERROR_CODE_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED         0x10
///<经常用的连接断开消息。就认为断开就行
#define ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION          0x13
///<正常的断开消息，本地L2CAP资源释放之后就会发这个值上来了
#define ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST        0x16

#define ERROR_CODE_ROLE_CHANGE_NOT_ALLOWED                    0x21

///杰理自定义，在回连的过程中使用page cancel命令主动取消成功会有这个命令
#define CUSTOM_BB_AUTO_CANCEL_PAGE                            0xFD  //// app cancle page
///杰理自定义，库里面有些功能需要停止page的时候会有。比如page的时候来了电话
#define BB_CANCEL_PAGE                                        0xFE  //// bb cancle page

#endif


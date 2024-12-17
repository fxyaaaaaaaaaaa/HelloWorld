#ifndef __104CLIENT___H___
#define __104CLIENT___H___

#include "../include/interface.h"
#include "../include/drvdefine.h"
#include "../include/rdbdefine.h"
#include "104define.h"
#include "public_protocols.h"

#define YK_NUM_YK_CTRL			16		// YK控制器个数
#define YK_STEP_NONE			0
#define YK_STEP_SELECT			1
#define YK_STEP_ACTIVE			2
//#define YK_STEP_CALCLE			3

#define _104CLIENT_DEFAULT_PRO_ATTR {0x4001, 0x01, 0x6401, 0x6001, 0x6201, 0x14, 0x45, 2, 2, 3, 300, 900, 0, 900, 8, 10, 1, 0, 1,0,10,C_SC_NA_1,C_DC_NA_1,0}
//规约参数配置，不能超过1024字节，对应_CHANNEL_INFO中的szProtocolAttribute
typedef struct tag104CLIENT_PRO_ATTR
{
	INT nYcBaseAddr;//0 遥测基地址
	INT nYxBaseAddr;//1 遥信基地址
	INT nYmBaseAddr; //2 遥脉基地址
	INT nYkBaseAddr; //3 遥控基地址
	INT nYtBaseAddr; //4 遥调基地址
	INT nZcallQOI; //5 总召唤QOI
	INT nDdZcallQCC; //6 电度总召唤QCC
	INT nAddrBytes; //7 地址域字节数
	INT nCotBytes; //8 传输原因字节数
	INT nInfoAddrBytes; //9 信息体地址字节长度
	INT nZcallPeriod; //10总召唤周期 单位：s
	INT nDdZcallPeriod;//11 电度总召唤周期 单位：s
	BOOL bTimeSync; //12 是否对时
	INT nTimeSyncPeriod; //13 对时周期 单位：s
	INT nIAffirmNum; //14 I帧确认频次
	INT nTestInterval; // 15 测试帧间隔
	BOOL bDoubleYk; // 16 是否双点遥控
	BOOL bSOEUpdateYx; //17 SOE是否更新遥信值
	BOOL bCOClearData; //18 通信中断数据（不含遥信）
	BOOL bCOClearYx; //19 通信中断遥信清零
	INT nCmmErrMax; //20 通信错误上限
	int nYkType;	// 21 单点遥控类型
	int nDubYkType;	// 22 双点遥控类型
	
	int nYcChgLimieCount;	//23 触发遥测变化限值启用采样次数。0 不限制
	BOOL bReadYt; // 24 召读遥调
	
	//char szRev[CH_PROTOCOL_ATTR_MAX_LEN - 80];	//此结构体长度不能计算错误,必须≤1024,否则会导致内存越界!!!
}_104CLIENT_PRO_ATTR,*P__104CLIENT_PRO_ATTR;
typedef struct {
	_TAG_INFO		yk_tag;
	TCP_CLIENT_TICK	*p_tcp;
	UCHAR			yk_step;	// YK当前执行的步骤
	UCHAR			yk_val;
	UCHAR			sz_none[2];
	
	time_t			yk_sec;		// YK执行的时间，用于超时处理


}IEC104_yk_ctrl_t;

typedef struct tagTCP_CHANNEL
{
	_TCP_IP_INFO *pMasterClient;
	_TCP_IP_INFO *pBackupClient;
	int nChannelID;
	RTCHANNEL *pAllChlInfo;
	USHORT usChannelAddr;
	int  nCommStatus;//104链路状态
	int	frame_gap_us;
	pthread_mutex_t mtx_yk_ctrl;//遥控控制器使用
	TCP_CLIENT_TICK *pTcpInfoList;//如果自己做client，它只有一个
	_104CLIENT_PRO_ATTR *pProAttr;//规约参数
	IEC104_yk_ctrl_t		sz_yk_ctrl[YK_NUM_YK_CTRL];		// 用于遥控处理
	int nStartWatch;//通道监测标记，开启计数+1，关闭-1，为0彻底关闭
	int nMsgLevel; //通道监测调试等级， 0不开启 1最简，2标准，3详细，默认2
	int	yt_scn;
	
	int	recnt_delay;
	BOOL can_call_yt;		//总招结束之后发
	struct tagTCP_CHANNEL *pNext;
}TCP_CHANNEL;

typedef struct {

	
	UINT	samp_count;		// 采样计数，数值过滤使用


}prv_104dev_t;

//自己做tcp server时用
typedef struct tagIEC104_DEV_DATA
{
	USHORT ushDevAddr;//底下报上来的104从站的RTU地址
	USHORT ushLastRecvLen;//最后一次接收报文剩下的长度
	//总召唤是否结束，只有总召唤结束了，才召唤电度
	//假如从站不规范，没有回复总召唤结束，那么会导致不发电度召唤
	BOOL bIsZCallDone;
	char szLastRecvBuff[1024];//最后一次接收报文剩下的数据

	YC_TAG_VALUE YC[MAX_YC_REG_LEN];
	YX_TAG_VALUE YX[MAX_YX_REG_LEN];
	YM_TAG_VALUE YM[MAX_YM_REG_LEN];
}_IEC104_DEV_DATA,*P_IEC104_DEV_DATA;




TCP_CHANNEL *GetChannelPtr(int nChannelID);
//以tcpclient的方式运行线程
void* iec104_client_thread(void* pParam);
//以tcpserver等待client连入
void* iec104_server_thread(void* pParam);
//初始化tcpclient
BOOL InitTcpSocket(TCP_CHANNEL *p_channel, _TCP_IP_INFO* pMasterClient, _TCP_IP_INFO* pBackupClient, int *hSokcet);
//解析数据
BOOL ParseRecvData(TCP_CHANNEL *pTCPChannel, UCHAR *szRecvBuf,int nLen,TCP_CLIENT_TICK *pClientTick);
//
BOOL Send(TCP_CHANNEL *pTCPChannel, char *Sendbuf, int num,TCP_CLIENT_TICK *pClientTick);
//发送测试帧
BOOL TestFrm(TCP_CHANNEL *pTCPChannel,TCP_CLIENT_TICK *pClientTick);
//启动激活
BOOL StartAct(TCP_CHANNEL *pTCPChannel,TCP_CLIENT_TICK *pClientTick);
//总召唤,自己做client的话pClientTick传NULL
BOOL ZCall(TCP_CHANNEL *pTCPChannel,TCP_CLIENT_TICK *pClientTick);
//电度总召唤
BOOL YM_ZCall(TCP_CHANNEL *pTCPChannel,TCP_CLIENT_TICK *pClientTick);

BOOL YT_ZCALL(TCP_CHANNEL *pTCPChannel,RTDEV *p_dev, TCP_CLIENT_TICK *pClientTick);

//对时
BOOL IEC104TimeSync(TCP_CHANNEL *pTCPChannel,TCP_CLIENT_TICK *pClientTick);
//遥控 bSC_DC 单点、双点 bSel_Exe 选择\执行
BOOL Contrl_YK(TCP_CHANNEL *pTCPChannel,USHORT usInfoAddr,
			   BOOL bSC_DC, BOOL bSel_Exe, BOOL bCancel, float fValue,TCP_CLIENT_TICK *pClientTick);
  //遥调
BOOL Contrl_YT(TCP_CHANNEL* pTCPChannel, _TAG_INFO *p_tag,USHORT usInfoAddr,
				BOOL bSel_Exe, BOOL bCancel,  float fValue,TCP_CLIENT_TICK *pClientTick);
//解析：不带品质的遥测量值
void ParseYC(TCP_CHANNEL *pTCPChannel, UCHAR *szRecvBuf,
			 int nBaseIndex, int nLen, UCHAR uchAsduVsq,TCP_CLIENT_TICK *pClientTick);
//解析：带品质、时标遥信量值
void ParseYX_Q_T(TCP_CHANNEL *pTCPChannel, UCHAR *szRecvBuf,
				 int nBaseIndex, int nLen, UCHAR uchAsduVsq,
				 BOOL bDbTag, BOOL bTime, BOOL b56Time,TCP_CLIENT_TICK *pClientTick);
//解析：带品质、时标遥测量值
void ParseYC_Q_T(TCP_CHANNEL *pTCPChannel, UCHAR *szRecvBuf,
				 int nBaseIndex, int nLen, UCHAR uchAsduVsq,
				 BOOL bTime, BOOL b56Time, BOOL bFloatValue,TCP_CLIENT_TICK *pClientTick);
//解析：带品质的遥脉量值
void ParseYM_Q_T(TCP_CHANNEL *pTCPChannel, UCHAR *szRecvBuf,
				 int nBaseIndex, int nLen, UCHAR uchAsduVsq,
				 BOOL bTime, BOOL b56Time,TCP_CLIENT_TICK *pClientTick);
//遍历client发送定时总召,传了pClient仅处理pClient，没传则遍历所有
void SendTimingZcall(TCP_CHANNEL *pTCPChannel,TCP_CLIENT_TICK *pClient);

BOOL add_client_by_ptr(TCP_CHANNEL* pChannelData,TCP_CLIENT_TICK **pClient);
BOOL delete_client_by_ptr(TCP_CHANNEL* pChannelData, TCP_CLIENT_TICK **pClient);
BOOL delete_client_by_fd(TCP_CHANNEL* pChannelData, int fdClient);
TCP_CLIENT_TICK* get_client_by_fd(TCP_CHANNEL* pChannelData, int fdClient);
int IEC104flash_client_state(TCP_CLIENT_TICK *pClientState, enum TCP_CLIENT_STATE eClientState);
//void close_sigint(int error);
//int IEC104send_data(int nChannelID, TCP_CLIENT_TICK *pClient, const int opt,const unsigned char* szSendBuf, const int nSendLen);

//简单等级报文解析
int simple_level_p2i(TCP_CHANNEL *pTCPChannel, UCHAR *p_pkt, int pkt_len, char *p_info, int info_len);
//标准等级报文解析
int standard_level_p2i(TCP_CHANNEL *pTCPChannel, UCHAR *p_pkt, int pkt_len, char *p_info, int info_len);
//详细等级报文解析
int detailed_level_p2i(TCP_CHANNEL *pTCPChannel, UCHAR *p_pkt, int pkt_len, char *p_info, int info_len);
//报文解析
void ChlWatchBuildContent(TCP_CHANNEL *pTCPChannel,USHORT usDataType,BOOL bIsHex,UCHAR* szBuf,USHORT usLen);
void ChlWatchPrint(TCP_CHANNEL *p_channel, int show_level, USHORT usDataType, char *format, ...);


void clear_data(TCP_CHANNEL *pTCPChannel);  //根据通信中断数据清除配置清除数据

//放置原因和公共地址到报文，原因和公共地址合法字节数1或2
void IEC104pkt_cot_comAddr_infoAddr(_104CLIENT_PRO_ATTR *pAttr, UCHAR *pPkt, int *pPktIdx, int cot, int addr, int infoAddr);
//放置信息体地址到报文，信息体地址合法字节数2或3
// void IEC104pkt_infoAddr(_104CLIENT_PRO_ATTR *pAttr, UCHAR *pPkt, int *pPktIdx, int infoAddr);

IEC104_yk_ctrl_t	*IEC104_get_free_yk_ctrl(TCP_CHANNEL *pTCPChannel);
IEC104_yk_ctrl_t	*IEC104_get_yk_ctrl_by_infoaddr(TCP_CHANNEL *pTCPChannel, int info_addr);
void IEC104_check_yk_timout(TCP_CHANNEL *pTCPChannel);



#endif //__104CLIENT___H___

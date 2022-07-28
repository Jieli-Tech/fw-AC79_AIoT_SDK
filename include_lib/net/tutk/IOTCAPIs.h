/*! \file IOTCAPIs.h
This file describes all the APIs of the IOTC module in IOTC platform.
IOTC module is a kind of data communication modules to provide basic data
transfer among devices and clients.

\copyright Copyright (c) 2010 by Throughtek Co., Ltd. All Rights Reserved.

Revision Table

Version     | Name             |Date           |Description
------------|------------------|---------------|-------------------
 */

#ifndef _IOTCAPIs_H_
#define _IOTCAPIs_H_

/* ============================================================================
 * Platform Dependant Macro Definition
 * ============================================================================
 */

#ifdef _WIN32
#ifdef IOTC_STATIC_LIB
#define P2PAPI_API
#elif defined P2PAPI_EXPORTS
#define P2PAPI_API __declspec(dllexport)
#else
#define P2PAPI_API __declspec(dllimport)
#endif
#else // #ifdef _WIN32
#define P2PAPI_API
#endif //#ifdef _WIN32


#ifndef _WIN32
#define __stdcall
#endif // #ifndef _WIN32


#ifdef _WIN32
/** @cond */


/** @endcond */
/** The default max number of IOTC sessions in IOTC module.
 * It is platform dependent and refer to source code for more detail. */
#define MAX_DEFAULT_IOTC_SESSION_NUMBER				128
#endif // #ifdef _WIN32

#if defined (__linux__) || defined (__APPLE__) || defined(__pnacl__)
#define MAX_DEFAULT_IOTC_SESSION_NUMBER				128
#endif // #ifdef __linux__  __APPLE__  __pnacl__

#ifdef IOTC_UCOSII
#define MAX_DEFAULT_IOTC_SESSION_NUMBER				8
extern unsigned int  TutkLoginTaskPri;
extern unsigned int  TutkRoutineTaskPri;
#endif // #ifdef IOTC_UCOSII

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ============================================================================
 * Generic Macro Definition
 * ============================================================================
 */
/** The maximum size, in byte, of the buffer used in IOTC_Session_Read(),
 * IOTC_Session_Write() and IOTC_Session_Read_Check_Lost(). */
#define IOTC_MAX_PACKET_SIZE						1388

/** The maximum size, in byte, of the buffer used in IOTC_Session_Write_Reliable_NB(). */
#define RELIABLE_MAX_PACKET_SIZE				    1384

/** The maximum number of IOTC channels for each IOTC session */
#define MAX_CHANNEL_NUMBER							32

/** The timeout, in unit of second, for keeping an IOTC session alive since
 * the last time remote site has response */
#define IOTC_SESSION_ALIVE_TIMEOUT					60

/** For IOTC Reliable checking send queue */
#define IOTC_RELIABLE_MSG_SEND_DONE 1
#define IOTC_RELIABLE_MSG_SENDING   0
/* ============================================================================
 * Error Code Declaration
 * ============================================================================
 */
/** The function is performed successfully. */
#define	IOTC_ER_NoERROR								0

/** IOTC servers have no response, probably caused by many types of Internet connection issues. */
#define	IOTC_ER_SERVER_NOT_RESPONSE					-1

/** IOTC masters cannot be resolved their domain name, probably caused
 * by network connection or DNS setting issues. */
#define	IOTC_ER_FAIL_RESOLVE_HOSTNAME				-2

/** IOTC module is already initialized. It is not necessary to re-initialize. */
#define IOTC_ER_ALREADY_INITIALIZED                 -3

/** IOTC module fails to create Mutexes when doing initialization. Please
 * check if OS has sufficient Mutexes for IOTC platform. */
#define IOTC_ER_FAIL_CREATE_MUTEX                   -4

/** IOTC module fails to create threads. Please check if OS has ability
 * to create threads for IOTC module. */
#define IOTC_ER_FAIL_CREATE_THREAD                  -5

/** IOTC module fails to create sockets. Please check if OS supports socket service */
#define IOTC_ER_FAIL_CREATE_SOCKET                  -6

/** IOTC module fails to set up socket options. */
#define IOTC_ER_FAIL_SOCKET_OPT                     -7

/** IOTC module fails to bind sockets */
#define IOTC_ER_FAIL_SOCKET_BIND                    -8

/** The specified UID is not licensed or expired. */
#define IOTC_ER_UNLICENSE                           -10

/** The device is already under login process currently
 * so it is prohibited to invoke login again at this moment. */
#define IOTC_ER_LOGIN_ALREADY_CALLED                -11

/** IOTC module is not initialized yet. Please use IOTC_Initialize() or
 * IOTC_Initialize2() for initialization. */
#define IOTC_ER_NOT_INITIALIZED                     -12

/** The specified timeout has expired during the execution of some IOTC
 * module service. For most cases, it is caused by slow response of remote
 * site or network connection issues */
#define IOTC_ER_TIMEOUT                             -13

/** The specified IOTC session ID is not valid. OR the SID of the session is closed.
 * A valid SID must be -1 < SID < max_session_number which is set by
 * IOTC_Set_Max_Session_Number and be got by IOTC_Connect_*. */
#define IOTC_ER_INVALID_SID                         -14

/** The specified device's name is unknown to the IOTC servers */
#define IOTC_ER_UNKNOWN_DEVICE                      -15

/** IOTC module fails to get the local IP address */
#define IOTC_ER_FAIL_GET_LOCAL_IP                   -16

/** The device already start to listen for connections from clients. It is
 * not necessary to listen again. */
#define IOTC_ER_LISTEN_ALREADY_CALLED               -17

/** The number of IOTC sessions has reached maximum.
 * To increase the max number of IOTC sessions, please use IOTC_Set_Max_Session_Number()
 * before initializing IOTC module. */
#define IOTC_ER_EXCEED_MAX_SESSION                  -18

/** IOTC servers cannot locate the specified device, probably caused by
 * disconnection from the device or that device does not login yet. */
#define IOTC_ER_CAN_NOT_FIND_DEVICE                 -19

/** The remote site already closes this IOTC session.
 * Please call IOTC_Session_Close() to release IOTC session resource in locate site. */
#define IOTC_ER_SESSION_CLOSE_BY_REMOTE             -22

/** This IOTC session is disconnected because remote site has no any response
 * after a specified timeout expires, i.e. #IOTC_SESSION_ALIVE_TIMEOUT */
#define IOTC_ER_REMOTE_TIMEOUT_DISCONNECT           -23

/** The client fails to connect to a device because the device is not listening for connections. */
#define IOTC_ER_DEVICE_NOT_LISTENING                -24

/** The IOTC channel of specified channel ID is not turned on before transferring data. */
#define IOTC_ER_CH_NOT_ON                           -26

/** A client stops connecting to a device by calling IOTC_Connect_Stop() */
#define IOTC_ER_FAIL_CONNECT_SEARCH                 -27

/** Too few masters are specified when initializing IOTC module.
 * Two masters are required for initialization at minimum. */
#define IOTC_ER_MASTER_TOO_FEW                      -28

/** A client fails to pass certification of a device due to incorrect key. */
#define IOTC_ER_AES_CERTIFY_FAIL                    -29

/** The number of IOTC channels for a IOTC session has reached maximum, say, #MAX_CHANNEL_NUMBER */
#define IOTC_ER_SESSION_NO_FREE_CHANNEL             -31

/** Cannot connect to masters neither UDP mode nor TCP mode by IP or host name ways */
#define IOTC_ER_TCP_TRAVEL_FAILED					-32

/** Cannot connect to IOTC servers in TCP */
#define IOTC_ER_TCP_CONNECT_TO_SERVER_FAILED        -33

/** A client wants to connect to a device in non-secure mode while that device
 * supports secure mode only. */
#define IOTC_ER_CLIENT_NOT_SECURE_MODE              -34

/** A client wants to connect to a device in secure mode while that device does
 * not support secure mode. */
#define IOTC_ER_CLIENT_SECURE_MODE					-35

/** A device does not support connection in secure mode */
#define IOTC_ER_DEVICE_NOT_SECURE_MODE              -36

/** A device does not support connection in non-secure mode */
#define IOTC_ER_DEVICE_SECURE_MODE					-37

/** A device stops listening for connections from clients. */
#define IOTC_ER_EXIT_LISTEN                         -39

/** The specified device does not support advance function
 *(TCP relay and P2PTunnel module) */
#define IOTC_ER_NO_PERMISSION                  		-40

/** Network is unreachable, please check the network settings */
#define	IOTC_ER_NETWORK_UNREACHABLE     			-41

/** A client fails to connect to a device via relay mode */
#define IOTC_ER_FAIL_SETUP_RELAY					-42

/** A client fails to use UDP relay mode to connect to a device
 * because UDP relay mode is not supported for that device by IOTC servers */
#define IOTC_ER_NOT_SUPPORT_RELAY					-43

/** No IOTC server information while device login or client connect
 * because no IOTC server is running or not add IOTC server list */
#define IOTC_ER_NO_SERVER_LIST						-44

/** The connecting device has duplicated loggin and may unconnectable. */
#define IOTC_ER_DEVICE_MULTI_LOGIN					-45

/** The arguments passed to a function is invalid. */
#define IOTC_ER_INVALID_ARG                         -46

/** The remote device not support partial encoding. */
#define IOTC_ER_NOT_SUPPORT_PE			            -47

/** The remote device no more free session can be connected. */
#define IOTC_ER_DEVICE_EXCEED_MAX_SESSION			-48


/** The function call is a blocking call and was called by other thread. */
#define IOTC_ER_BLOCKED_CALL						-49

/** The session was closed. */
#define IOTC_ER_SESSION_CLOSED						-50

/** Remote doesn't support this function. */
#define IOTC_ER_REMOTE_NOT_SUPPORTED				-51

/** The function is aborted by related function. */
#define IOTC_ER_ABORTED								-52

/** The buffer size exceed maximum packet size. */
#define IOTC_ER_EXCEED_MAX_PACKET_SIZE				-53

/** Server does not support this feature. */
#define IOTC_ER_SERVER_NOT_SUPPORT					-54

/** Cannot find a path to write data*/
#define IOTC_ER_NO_PATH_TO_WRITE_DATA				-55

/** Start function is not called */
#define IOTC_ER_SERVICE_IS_NOT_STARTED  			-56

/** Already in processing*/
#define IOTC_ER_STILL_IN_PROCESSING                 -57

/** Out of memory*/
#define IOTC_ER_NOT_ENOUGH_MEMORY                   -58

/** The device is banned and locked, this error code is no longer being used*/
#define IOTC_ER_DEVICE_IS_BANNED					-59

/** IOTC master servers have no response, probably caused by many types of Internet connection issues. */
#define IOTC_ER_MASTER_NOT_RESPONSE					-60

/** IOTC module has some resource allocating problem. */
#define IOTC_ER_RESOURCE_ERROR						-61

/** IOTC Write reliable send queue is full. */
#define IOTC_ER_QUEUE_FULL							-62

/** The feature is not supported. */
#define IOTC_ER_NOT_SUPPORT							-63

/** Device is in sleep mode. */
#define IOTC_ER_DEVICE_IS_SLEEP						-64

/** Device doesn't support this feature on TCP mode. */
#define IOTC_ER_TCP_NOT_SUPPORT						-65

/** IOTC_WakeUp_Init isn't called */
#define IOTC_ER_WAKEUP_NOT_INITIALIZED              -66

/** All Server response can not find device */
#define IOTC_ER_DEVICE_OFFLINE						-90

/**	IOTC master server is invalid */
#define IOTC_ER_MASTER_INVALID						-91

/* ============================================================================
 * Enumeration Declaration
 * ============================================================================
 */

/**
 * \details IOTC session mode, used in IOTC_Listen2(), IOTC_Connect_ByUID2()
 *			to specify what kinds of IOTC session
 *			that devices will listen or clients will connect.
 */
typedef enum {
    /// IOTC session will be established in non-secure mode. <br>
    /// - For devices, the behavior is the same as IOTC_Listen(). <br>
    /// - For clients, the behavior is the same as IOTC_Connect_ByUID()
    IOTC_NON_SECURE_MODE = 0,

    /// IOTC session will be established in secure mode. <br>
    /// - For devices, it means only secure connection will be accepted. <br>
    /// - For clients, it means only secure connection will be performed
    IOTC_SECURE_MODE = 1,

    /// IOTC session will be established in either non-secure or secure mode,
    /// depending on remote site's request. Can use IOTC_Session_Check()
    /// to check what mode is actually used. <br>
    /// - For devices, it means both non-secure or secure modes are accepted. <br>
    /// - For clients, it means either non-secure or secure modes can be performed.
    IOTC_ARBITRARY_MODE = 2
} IOTCSessionMode;


/**
 * \detials IOTC Device Login states which indicates the state of a device login process.
 * IOTC_Device_Login_NB is a non-blocking function which means it returns
 * immediately. The state will be shown in loginStateHandler which needs to
 * be assigned when calling IOTC_Device_Login_NB.
 */
typedef enum {
    IOTC_DEVLOGIN_ST_START             = 1 << 0, // start to doing login.
    IOTC_DEVLOGIN_ST_READY_FOR_LAN     = 1 << 1, // The device is able to be reached in LAN mode.
    IOTC_DEVLOGIN_ST_LOGINING          = 1 << 2, // Still try to login.
    IOTC_DEVLOGIN_ST_LOGINED           = 1 << 3, // get the login response from IOTC servers.
    IOTC_DEVLOGIN_ST_RELOGINING        = 1 << 4,
    IOTC_DEVLOGIN_ST_MULTI_LOGIN       = 1 << 5,
    IOTC_DEVLOGIN_ST_LOGIN_FAILED      = -1,     //Login failed. check the error code.

} IOTCDeviceLoginState;

/**
 * \detials IOTC Client Connection State which shows the state of a Client
 * connects to a device. IOTC_Connect_ByUIDNB and IOTC_Connect_ByUID_ParallelNB
 * are non-blocking calls. This state should be checked after the two functions
 * are called. The state will be shown in connectStateHandler which needs to
 * be assigned when calling IOTC_Connect_ByUIDNB or IOTC_Connect_ByUID_ParallelNB.
 */
typedef enum {
    IOTC_CONNECT_UID_ST_FAILED = -1, // A Client connects to a device failed.
    IOTC_CONNECT_UID_ST_START = 0,   // A Client start connecting to a Device.
    IOTC_CONNECT_UID_ST_CONNECTING,  // A Client is connecting a device.
    IOTC_CONNECT_UID_ST_CONNECTED    // The connection are established between a Client and a Device.
} IOTCConnectState;

typedef enum {
    IOTC_DEV_ST_LOGIN_FAILED = -2,       // The device fails to login.
    IOTC_DEV_ST_ERROR = -1,				 // An error occurs in the device.
    IOTC_DEV_ST_UNINITIALIZED = 0,       // The device has not been initialized.
    IOTC_DEV_ST_INITIALIZED,             // The device has been initialized.
    IOTC_DEV_ST_LOGINING,                // The device is doing login process.
    IOTC_DEV_ST_LOGINED,                 // The device has logined to a P2P server. Also waiting for a client to connect.
    IOTC_DEV_ST_CONNECTING,				 // A Client is connecting to the device.
    IOTC_DEV_ST_CONNECTED_WAITING,		 // One or more clients has been connected to the device.
    // And the device still waits for more clients to connect.
} IOTCDeviceState;

/* ============================================================================
 * Structure Definition
 * ============================================================================
 */

/**
 * \details IOTC session info, containing all the information when a IOTC session is
 *  established between a device and a client. Users can use IOTC_Session_Check()
 *  to get IOTC session information.
 */
struct st_SInfo {
    unsigned char Mode; //!< 0: P2P mode, 1: Relay mode, 2: LAN mode
    char CorD; //!< 0: As a Client, 1: As a Device
    char UID[21]; //!< The UID of the device
    char RemoteIP[47]; //!< The IP address of remote site used during this IOTC session
    unsigned short RemotePort; //!< The port number of remote site used during this IOTC session
    unsigned int TX_Packetcount; //!< The total packets sent from the device and the client during this IOTC session
    unsigned int RX_Packetcount; //!< The total packets received in the device and the client during this IOTC session
    unsigned int IOTCVersion; //!< The IOTC version
    unsigned short VID; //!< The Vendor ID, part of VPG mechanism
    unsigned short PID; //!< The Product ID, part of VPG mechanism
    unsigned short GID; //!< The Group ID, part of VPG mechanism
    unsigned char NatType; //!< The remote NAT type
    unsigned char isSecure; //!< 0: The IOTC session is in non-secure mode, 1: The IOTC session is in secure mode
};

typedef struct st_SInfo st_SInfo_t;

/**
 * \details IOTC session info, containing all the information when a IOTC session is
 *  established between a device and a client. Users can use IOTC_Session_Check_Ex()
 *  to get IOTC session information.
 */
struct st_SInfoEx {
    unsigned int size; //!< size of the structure
    unsigned char Mode; //!< 0: P2P mode, 1: Relay mode, 2: LAN mode, 255: Not connected.
    char CorD; //!< 0: As a Client, 1: As a Device
    char UID[21]; //!< The UID of the device
    char RemoteIP[47]; //!< The IP address of remote site used during this IOTC session
    unsigned short RemotePort; //!< The port number of remote site used during this IOTC session
    unsigned int TX_Packetcount; //!< The total packets sent from the device and the client during this IOTC session
    unsigned int RX_Packetcount; //!< The total packets received in the device and the client during this IOTC session
    unsigned int IOTCVersion; //!< The IOTC version
    unsigned short VID; //!< The Vendor ID, part of VPG mechanism
    unsigned short PID; //!< The Product ID, part of VPG mechanism
    unsigned short GID; //!< The Group ID, part of VPG mechanism
    unsigned char isSecure; //!< 0: The IOTC session is in non-secure mode, 1: The IOTC session is in secure mode
    unsigned char LocalNatType; //!< The local NAT type, 0: Unknown type, 1: Type 1, 2: Type 2, 3: Type 3, 10: TCP only
    unsigned char RemoteNatType; //!< The remote NAT type, 0: Unknown type, 1: Type 1, 2: Type 2, 3: Type 3, 10: TCP only
    unsigned char RelayType; //!< 0: Not Relay, 1: UDP Relay, 2: TCP Relay
    unsigned int NetState; //!<If no UDP packet is ever received, the first bit of value is 1, otherwise 0
    char RemoteWANIP[47]; //!< The WAN IP address of remote site used during this IOTC session and it is only valid in P2P or Relay mode
    unsigned short RemoteWANPort; //!< The WAN port number of remote site used during this IOTC session and it is only valid in P2P or Relay mode
};

typedef struct st_SInfoEx st_SInfoEx_t;

/**
 * \details Device search info, containing all the information
 * when client searches devices in LAN.
 */
struct st_LanSearchInfo {
    char UID[21]; //!< The UID of discovered device
    char IP[16]; //!< The IP address of discovered device
    unsigned short port; //!< The port number of discovered device used for IOTC session connection
    char Reserved; //!< Reserved, no use
};

typedef struct st_LanSearchInfo st_LanSearchInfo_t;

/**
 * \details Device search info, containing all the information and device name
 * when client searches devices in LAN.
 */
struct st_LanSearchInfo2 {
    char UID[21]; //!< The UID of discovered device
    char IP[16]; //!< The IP address of discovered device
    unsigned short port; //!< The port number of discovered device used for IOTC session connection
    char DeviceName[132]; //!< The Name of discovered device
    char Reserved; //!< Reserved, no use
};

typedef struct st_LanSearchInfo2 st_LanSearchInfo2_t;

/**
 * \details Device search info, containing all the information and device name
 * when client searches devices in LAN.
 */
struct st_SearchDeviceInfo {
    char UID[21]; //!< The UID of discovered device
    char IP[46]; //!< The IP address of discovered device
    unsigned short port; //!< The port number of discovered device used for IOTC session connection
    char DeviceName[132]; //!< The Name of discovered device
    char Reserved; //!< Reserved, no use
    int  nNew; //!< New device
};

typedef struct st_SearchDeviceInfo st_SearchDeviceInfo_t;

/**
 * \details Connect Option, containing all options of connection setup when client connects to
 * device by P2P or relay mode.
 */
struct st_ConnectOption {
    char IsParallel; //!< 0: Turn off parallel connection. 1: Turn on parallel connection.
    char IsLowConnectionBandwidth; //!< 0: Normal connection mode 1: Low bandwidth mode (This mode might reduce the P2P traversal rate)
    char IsP2PRequestRoundRobin; //!< 0: Normal connection mode 1: P2P Request round robin mode (This mode might reduce the P2P traversal rate)
    char IsNotToCheckLanIpforP2P; //!< 0: Check the P2P connected IP if it is change mode to LAN 1: Do not check the P2P connected IP
};

typedef struct st_ConnectOption st_ConnectOption_t;

struct st_DeviceStInfo {
    IOTCDeviceState state;
    int connectedSessionNum;  // How many clients are connected to the device now.
//	int connectedSessionList[MAX_SESSION_NUMBER]; // The connected session id list.
    int newConnected;  // One or more new clients are connected.
    int errCode;       // What kind of error happens once the device is in IOTC_DEV_ST_ERROR state.
};

typedef struct st_DeviceStInfo st_DeviceStInfo_t;

/* ============================================================================
 * Type Definition
 * ============================================================================
 */

/**
 * \details The prototype of getting login info function, used by a device
 *			to be notified if it is still kept login with IOTC servers or is
 *			disconnected with IOTC servers.
 *
 * \param pnLoginInfo [out] The login info with meanings of following bits
 *			- bit 0: the device is ready for connection by client from LAN if this bit is 1
 *			- bit 1: once the device can contact with masters and p2p servers, this bit will be set as 1
 *			- bit 2: if this bit is 1, it means the device has received login
 *						response from IOTC servers since IOTC_Get_Login_Info()
 *						is called last time. This bit will be set as 0 if p2p server has not
 *						responsed about 75 seconds.
 *
 */
typedef void (__stdcall *loginInfoCB)(unsigned int nLoginInfo);


/**
 * \details This handler is used for IOTC_Device_Login_NB. It will be called when the login state
 * is changed. So that the user is able to check if a device registers itself to the P2P server
 * successfully.
 *
 * \attention This handler SHOULD NOT be blocked for any reason.
 *
 * \param state [out] The device login state. refer to IOTCDeviceLoginState for more information.
 *        errCode [out] The error code represents what the problems it during the device login. To be noted,
 *        This value is meaningful when state is IOTC_DEVLOGIN_ST_LOGIN_FAILED.
 *          - #IOTC_ER_NoERROR if device had obtained server list and sent login packets successfully.
 *			- #IOTC_ER_UNLICENSE The UID is not licensed or expired
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_LOGIN_ALREADY_CALLED The device is already under login process currently
 *			- #IOTC_ER_FAIL_CREATE_SOCKET Fails to create sockets
 *			- #IOTC_ER_FAIL_RESOLVE_HOSTNAME Cannot resolve masters' host name
 *			- #IOTC_ER_NO_PERMISSION The device does not support TCP relay
 *			- #IOTC_ER_FAIL_GET_LOCAL_IP Fails to get the device's local IP address
 *			- #IOTC_ER_NETWORK_UNREACHABLE Network is unreachable
 *			- #IOTC_ER_RESOURCE_ERROR Getting system resource fail. (i.e. socket, thread etc.)
 *			- #IOTC_ER_MASTER_NOT_RESPONSE All masters have no respond
 *			- #IOTC_ER_TCP_TRAVEL_FAILED Cannot connect to masters in neither UDP nor TCP
 *			- #IOTC_ER_TCP_CONNECT_TO_SERVER_FAILED Cannot connect to IOTC servers in TCP
 *
 *        pUserData [out] The user date which is passed from IOTC_Device_Login_NB call.
 *
 */
typedef void (*loginStateHandler)(IOTCDeviceLoginState state, int errCode, void *pUserData);


/**
 * \details This handler is used for IOTC_Connect_ByUIDNB and IOTC_Connect_ByUID_ParallelNB.
 * It will be called when the connect state is changed. So that the user is able to check
 * if a client connects to a device successfully or not.
 *
 * \attention This handler SHOULD NOT be blocked for any reason.
 *
 * \param state [out] The connect state. refer to IOTCConnectState for more information.
 *        errCode [out] The error code represents what the problems it during the connect. To be noted,
 *        This value is meaningful when state is IOTC_CONNECT_UID_ST_FAILED.
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_UNLICENSE The specified UID of that device is not licensed or expired
 *			- #IOTC_ER_EXCEED_MAX_SESSION The number of IOTC sessions has reached maximum in client side
 *			- #IOTC_ER_DEVICE_NOT_LISTENING The device is not listening for connection now
 *			- #IOTC_ER_FAIL_CONNECT_SEARCH The client stop connecting to the device
 *			- #IOTC_ER_FAIL_CREATE_THREAD Fails to create threads
 *			- #IOTC_ER_CAN_NOT_FIND_DEVICE IOTC servers cannot locate the specified device
 *			- #IOTC_ER_FAIL_GET_LOCAL_IP Fails to get the client's local IP address
 *			- #IOTC_ER_FAIL_SETUP_RELAY Fails to connect the device via relay mode
 *			- #IOTC_ER_FAIL_CREATE_SOCKET Fails to create sockets
 *			- #IOTC_ER_NOT_SUPPORT_RELAY Not support relay connection by IOTC servers
 *			- #IOTC_ER_MASTER_NOT_RESPONSE All masters have no respond
 *			- #IOTC_ER_TCP_TRAVEL_FAILED Cannot connect to masters in neither UDP nor TCP
 *			- #IOTC_ER_TCP_CONNECT_TO_SERVER_FAILED Cannot connect to IOTC servers in TCP
 *
 *        pUserData [out] The user date which is passed from IOTC_Connect_ByUIDNB or IOTC_Connect_ByUID_ParallelNB call.
 *
 */
typedef void (*connectStateHandler)(IOTCConnectState state, int errCode, void *pUserData);

/**
 * \details The prototype of getting session status function, used by a device
 *			or client to be notified if session is disconnected.
 *
 * \param nIOTCSessionID [out] The session ID of the session being disconnected
 * \param nErrorCode [out]
 *			- #IOTC_ER_SESSION_CLOSE_BY_REMOTE The IOTC session of specified
 *				session ID has been closed by remote site
 *			- #IOTC_ER_REMOTE_TIMEOUT_DISCONNECT The timeout defined by #IOTC_SESSION_ALIVE_TIMEOUT
 *				expires because	remote site has no response
 *
 */
typedef void (__stdcall *sessionStatusCB)(int nIOTCSessionID, int nErrorCode);

/**
 * \details The prototype of connection mode change function, used by a device
 *			or client to be notified if connection mode is changed.
 *
 * \param nIOTCSessionID [out] return effected session ID
 * \param nConnMode 	 [out] return the current connection mode
 * 			- # nConnMode = 0 : LAN Mode
 * 			- # nConnMode = 1 : P2P Mode
 * 			- # nConnMode = 2 : UDP RLY Mode
 * 			- # nConnMode = 3 : TCP RLY Mode
 */
typedef void(__stdcall *ConnectModeChangeCB)(int nIOTCSessionID, unsigned int nConnMode);

/**
 * \details This is the handler for reporting the device on line status. It is used for
 * IOTC_Check_Device_On_Line().
 *
 * \param result [out] The result of getting the device status.
 *			- #IOTC_ER_NoERROR No error happens during the checking flow and the Device is on line.
 *			- #IOTC_ER_NETWORK_UNREACHABLE Network is unreachable
 *			- #IOTC_ER_MASTER_NOT_RESPONSE All masters have no respond
 *			- #IOTC_ER_TCP_CONNECT_TO_SERVER_FAILED Cannot connect to IOTC servers in TCP
 *			- #IOTC_ER_CAN_NOT_FIND_DEVICE IOTC servers cannot locate the specified device
 *			- #IOTC_ER_SERVER_NOT_RESPONSE All servers have no response
 *			- #IOTC_ER_TCP_TRAVEL_FAILED Cannot connect to masters in neither UDP nor TCP
 *			- #IOTC_ER_DEVICE_OFFLINE The device is not on line.
 *
 * \param userData [in] The data which was passed during IOTC_Check_Device_On_Line() is called.
 *
 */
typedef void(__stdcall *onLineResult)(int result, void *userData);

/* ============================================================================
 * Function Declaration
 * ============================================================================
 */

/**
 * \brief Get the version of IOTC module
 *
 * \details This function returns the version of IOTC module.
 *
 * \param pnVersion [out] The version of IOTC module. It contains
 *			the version from high byte to low byte, for example, 0x01020304
 *			means the version is 1.2.3.4
 *
 * \see RDT_GetRDTApiVer(), avGetAVApiVer()
 */
P2PAPI_API void IOTC_Get_Version(unsigned int *pnVersion);


/**
 * \brief Set the max number of IOTC sessions of IOTC module
 *
 * \details This function set the max number of allowable IOTC sessions in IOTC
 *			module. The max number of IOTC session limits the max number of
 *			connected clients in device side, while it limits the max number
 *			connected devices in client	side. A device or a client could use
 *			this function to reduce the number of IOTC sessions in order to save
 *			some memory usage.
 *
 * \param nMaxSessionNum [in] The max number of IOTC sessions
 *
 * \attention	(1) This function is optional if users do not want to change the
 *				default max number of IOTC sessions, i.e. MAX_DEFAULT_IOTC_SESSION_NUMBER.
 *				However, if users really wants to change it, this function
 *				shall be called before IOTC platform is initialized.<br><br>
 *				(2) The maximum IOTC session number is platform dependent.
 *				See the definition of MAX_DEFAULT_IOTC_SESSION_NUMBER for each
 *				platform.
 */
P2PAPI_API void IOTC_Set_Max_Session_Number(unsigned int nMaxSessionNum);


/**
 * \brief Initialize IOTC module
 *
 * \details **This function is deprecated, you should use IOTC_Initialize instead.**
 *          This function is used by devices or clients to initialize IOTC
 *			module and shall be called before any IOTC module related
 *			function is invoked except for IOTC_Set_Max_Session_Number().
 *
 * \param nUDPPort [in] Specify a UDP port. Random UDP port is used if it is specified as 0.
 * \param cszP2PHostNamePrimary [in] Specify the host name or IP address of
 *			the primary master. Cannot be NULL. See attention below for more detail.
 * \param cszP2PHostNameSecondary [in] Specify the host name or IP address of
 *			the secondary master. Cannot be NULL. See attention below for more detail.
 * \param cszP2PHostNameThird [in] Specify the host name or IP address of
 *			the third master. Can be NULL if only two masters are required.
 *			See attention below for more detail.
 * \param cszP2PHostNameFourth [in] Specify the host name or IP address of
 *			the fourth master. Can be NULL if only two masters are required.
 *			See attention below for more detail.
 *
 * \return #IOTC_ER_NoERROR if initializing successfully
 * \return Error code if return value < 0
 *			- #IOTC_ER_FAIL_RESOLVE_HOSTNAME Cannot resolve masters' host name
 *			- #IOTC_ER_ALREADY_INITIALIZED IOTC module is already initialized
 *			- #IOTC_ER_MASTER_TOO_FEW Two masters are required as parameters at minimum
 *			- #IOTC_ER_FAIL_CREATE_MUTEX Fails to create Mutexs
 *			- #IOTC_ER_FAIL_CREATE_THREAD Fails to create threads
 *			- #IOTC_ER_NOT_ENOUGH_MEMORY not enough memory
 *
 * \see IOTC_Initialize2(), IOTC_DeInitialize()
 *
 * \attention   (1) This function is the key entry to whole IOTC platform, including
 *				RDT module and AV module. That means, if you want to use
 *				RDT module, users shall still use this function to initialize IOTC
 *				module before calling RDT_Initialize(). <br><br>
 *				(2) Usually, host name is suggested to be used to specify a master.
 *				because that will ensure devices and clients can still connect
 *				to masters even the network address configuration of masters
 *				changes in the future. However, in rare case, the host name
 *				of masters can not be resolved due to network issue and it is
 *				necessary to specify IP address of masters in this function
 *				in order for successful connection. The host name and IP address
 *				of each master is listed as below:
 *				- Master #1: m1.iotcplatform.com => 50.19.254.134
 *				- Master #2: m2.iotcplatform.com => 122.248.234.207
 *				- Master #3: m3.iotcplatform.com => 46.137.188.54
 *				- Master #4: m4.iotcplatform.com => 122.226.84.253
 *				- Master #5: m5.iotcplatform.com => 61.188.37.216
 *				- Master #6: m6.iotcplatform.com => 120.24.59.150
 *				- Master #7: m7.iotcplatform.com => 114.215.137.159
 *				- Master #8: m8.iotcplatform.com => 104.199.156.58
 */
P2PAPI_API int IOTC_Initialize(unsigned short nUDPPort, const char *cszP2PHostNamePrimary,
                               const char *cszP2PHostNameSecondary, const char *cszP2PHostNameThird,
                               const char *cszP2PHostNameFourth);


/**
 * \brief Initialize IOTC module
 *
 * \details This function is used by devices or clients to initialize IOTC
 *			module and shall be called before	any IOTC module related
 *			function is invoked except for IOTC_Set_Max_Session_Number().
 *			<br>
 *			The different between this function and IOTC_Initialize() is this
 *			function uses following steps to connect masters (1) IP addresses
 *			of master (2) if fails to connect in step 1, resolve predefined
 *			domain name of masters (3) try to connect again with the resolved
 *			IP address of step 2 if IP is resolved successfully.
 *
 * \param nUDPPort [in] Specify a UDP port. Random UDP port is used if it is specified as 0.
 *
 * \return #IOTC_ER_NoERROR if initializing successfully
 * \return Error code if return value < 0
 *			- #IOTC_ER_ALREADY_INITIALIZED IOTC module is already initialized
 *			- #IOTC_ER_FAIL_CREATE_MUTEX Fails to create Mutexes
 *			- #IOTC_ER_FAIL_CREATE_THREAD Fails to create threads
 *			- #IOTC_ER_NOT_ENOUGH_MEMORY not enough memory
 *
 * \see IOTC_Initialize(), IOTC_DeInitialize()
 *
 * \attention This function is the key entry to whole IOTC platform, including
 *				RDT module and AV module. That means, if you want to use
 *				RDT module, users shall still use this function to initialize IOTC
 *				module before calling RDT_Initialize().
 */
P2PAPI_API int IOTC_Initialize2(unsigned short nUDPPort);


/**
 * \brief Deinitialize IOTC module
 *
 * \details This function will deinitialize IOTC module.
 *
 * \return #IOTC_ER_NoERROR if deinitialize successfully
 * \return Error code if return value < 0
 *			- #IOTC_ER_NOT_INITIALIZED the IOTC module is not initialized yet.
 *
 * \see IOTC_Initialize(), IOTC_Initialize2()
 *
 * \attention IOTC_DeInitialize() will automatically close all IOTC sessions
 *				in local site while the remote site will find sessions have
 *				been closed after certain period of time. Therefore, it is
 *				suggested to close all sessions before invoking this function
 *				to ensure the remote site and real-time session status.
 */
P2PAPI_API int IOTC_DeInitialize(void);


/**
 * \brief Used by a device to login to IOTC servers
 *
 * \details This function will let a device login to IOTC servers. UID is required
 *			when login to IOTC servers. The device name and password are applicable
 *			only in LAN mode when the device cannot login to IOTC servers
 *			due to network issue.
 *
 * \param cszUID [in] The UID of that device login to IOTC servers
 * \param cszDeviceName [in] The name of that device, used in LAN mode for clients
 *			to connect
 * \param cszDevicePWD [in] This parameter is no longer used
 *
 * \return #IOTC_ER_NoERROR if device had obtained server list and sent login packets successfully
 * \return Error code if return value < 0
 *			- #IOTC_ER_UNLICENSE The UID is not licensed or expired
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_LOGIN_ALREADY_CALLED The device is already under login process currently
 *			- #IOTC_ER_FAIL_CREATE_SOCKET Fails to create sockets
 *			- #IOTC_ER_FAIL_SOCKET_OPT Fails to set up socket options
 *			- #IOTC_ER_FAIL_SOCKET_BIND Fails to bind sockets
 *			- #IOTC_ER_FAIL_CREATE_THREAD Fails to create threads
 *			- #IOTC_ER_FAIL_RESOLVE_HOSTNAME Cannot resolve masters' host name
 *			- #IOTC_ER_TCP_TRAVEL_FAILED Cannot connect to masters in neither UDP nor TCP
 *			- #IOTC_ER_TCP_CONNECT_TO_SERVER_FAILED Cannot connect to IOTC servers in TCP
 *			- #IOTC_ER_NO_PERMISSION The device does not support TCP relay
 *			- #IOTC_ER_SERVER_NOT_RESPONSE All servers have no response
 *			- #IOTC_ER_FAIL_GET_LOCAL_IP Fails to get the device's local IP address
 *			- #IOTC_ER_NETWORK_UNREACHABLE Network is unreachable
 *			- #IOTC_ER_NO_SERVER_LIST No IOTC server information while device login
 *			- #IOTC_ER_MASTER_NOT_RESPONSE All masters have no respond
 *
 * \attention (1) This function is a block process. That means this function will return until
 *				the device can login into IOTC servers successfully or some error happens
 *				during the process. It is suggested to use another thread to perform
 *				login process so that sequential instructions will not blocked. <br><br>
 *				(2) Please use IOTC_Get_Login_Info() to check if device really logins
 * 				into server successfully.
 */
P2PAPI_API int IOTC_Device_Login(const char *cszUID, const char *cszDeviceName, const char *cszDevicePWD);


/**
 * \brief Used by a device to login to IOTC servers
 *
 * \details This function is used for registering a device to IOTC servers which
 *			is called login process. During login a UID is a mandatory argument
 *			for inditifing the device. The device name and password are applicable
 *			only in LAN mode when the device cannot login to IOTC servers
 *			due to network issue. This is a non-blocking call. It triggers
 *			the login process. Therefore a state handler is needed to take care
 *			all the states. This handler will be called once the state is changed.
 *
 * \param cszUID [in] The UID of that device login to IOTC servers
 * \param cszDeviceName [in] The name of that device, used in LAN mode for clients
 *			to connect
 * \param cszDevicePWD [in] The password of that device, used in LAN mode for
 *			clients to connect
 * \param hadler [in] The login state handler which will be called once the login
 *					  state is changed.
 * \param userData [in] The user data which will be brought to the handler.
 *
 * \return #IOTC_ER_NoERROR if device had obtained server list and sent login packets successfully
 *			- #IOTC_ER_UNLICENSE The UID is not licensed or expired
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_LOGIN_ALREADY_CALLED The device is already under login process currently
 *			- #IOTC_ER_FAIL_CREATE_SOCKET Fails to create sockets
 *			- #IOTC_ER_FAIL_RESOLVE_HOSTNAME Cannot resolve masters' host name
 *			- #IOTC_ER_NO_PERMISSION The device does not support TCP relay
 *			- #IOTC_ER_FAIL_GET_LOCAL_IP Fails to get the device's local IP address
 *			- #IOTC_ER_NETWORK_UNREACHABLE Network is unreachable
 *			- #IOTC_ER_RESOURCE_ERROR Getting system resource fail. (i.e. socket, thread etc.)
 */
P2PAPI_API int IOTC_Device_LoginNB(const char *cszUID, const char *cszDeviceName, const char *cszDevicePWD,
                                   loginStateHandler handler, void *userData);


/**
 * \brief Used by a device to get the login information
 *
 * \details This function gets the login information of a device to IOTC servers.
 *
 * \param pnLoginInfo [out] The login info with meanings of following bits
 *			- bit 0: the device is ready for connection by client from LAN if this bit is 1
 *			- bit 1: the device is ready for connection by client from Internet if this bit is 1
 *			- bit 2: if this bit is 1, it means the device has received login
 *						response from IOTC servers since IOTC_Get_Login_Info()
 *						is called last time.
 *
 * \return The number of fails to login to IOTC servers.
 * \return Error code if return value < 0
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *
 */
P2PAPI_API int IOTC_Get_Login_Info(unsigned int *pnLoginInfo);


/**
 * \brief Checking a device on line or not.
 *
 * \details This function is used for checking a device on line status. A device
 * is on line means the device login to a server or the device is able to be searched
 * on LAN. To check to state, it takes a little bit time to query the device status
 * from IOTC servers. In order to not blocking anything, the function returns
 * immediately, the result will be shown in the handler.
 *
 * \attention THE "hander" MUST NOT BE BLOCKED. IT SHOULD BE RETURNED ASAP.
 * IF YOU WANT TO DO SOMETHING BLOCK (i.e. sleep, or any kind of loop.),
 * PLEASE CREATE A THREAD TO DO IT.
 *
 * \param UID [in] A device UID which is used to check the state.
 * \param timeOut [in] The time out value of checking device information in millisecond.
 * \param handler [in] A handle function for getting the device on line result.
 * \param userData [in] The data would like to bring to the handler for feather processing.
 *
 *
 * \return IOTC_ER_NoERROR on successful. The others are error.
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- IOTC_ER_UNLICENSE The UID is not a valid UID.
 *			- IOTC_ER_EXCEED_MAX_SESSION The session is full.
 *			- #IOTC_ER_FAIL_CREATE_SOCKET Fails to create sockets
 *			- #IOTC_ER_FAIL_SOCKET_OPT Fails to set up socket options
 *			- #IOTC_ER_FAIL_SOCKET_BIND Fails to bind sockets
 *			- IOTC_ER_NOT_ENOUGH_MEMORY No enough memory to run the function.
 *
 */
P2PAPI_API int IOTC_Check_Device_On_Line(const char *UID, const unsigned int timeOut,
        onLineResult handler, void *userData);


/**
 * \brief Used by a device to get the login information
 *
 * \details This function gets the login information of a device to IOTC servers. <br>
 *			The difference of this function and IOTC_Get_Login_Info() is
 *			this function will set callback function inside IOTC module and
 *			that callback function will be invoked whenever the login status
 *			of that device is updated from IOTC servers, for example, IOTC
 *			servers response login message to that device or the connection
 *			between IOTC servers and that device has been lost.
 *
 * \param pfxLoginInfoFn [in] The function pointer to getting login info function
 *
 */
P2PAPI_API void IOTC_Get_Login_Info_ByCallBackFn(loginInfoCB pfxLoginInfoFn);


/**
 * \brief This function is used for getting device status information.
 *
 * \details Getting device status information to know what device is doing.
 *			Also to know how many sesstion has been used and if there is a
 *			new client is connected. Usually, this function can be used with
 *			IOTC_Accept(). First all, calling IOTC_Get_Device_Status to check if
 *			a new client is connected. If so, calling IOTC_Accept to get the
 *			new session id. To be noted, the memory that the argument pDevStInfo
 *			points to will be clear to 0 in the function before getting the
 *			information.
 *
 * \param pDevStInfo [out] A pointer that points to a memory which the device
 *						   information is saved to.
 *
 * \return IOTC_ER_NoERROR on success. Otherwise it fails.
 */

P2PAPI_API int IOTC_Get_Device_Status(struct st_DeviceStInfo *pDevStInfo);


/**
 * \brief Used by a device to listen connections from clients
 *
 * \details This function is for a device to listen any connection from clients.
 *			If connection is established with the help of IOTC servers, the
 *			IOTC session ID will be returned in this function and then device and
 *			client can communicate for the other later by using this IOTC session ID.
 *
 * \param nTimeout [in] The timeout for this function in unit of millisecond, give 0 means block forever
 *
 * \return IOTC session ID if return value >= 0
 * \return Error code if return value < 0
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_EXCEED_MAX_SESSION The number of IOTC sessions has reached maximum in device side
 *			- #IOTC_ER_LISTEN_ALREADY_CALLED The device is already in listen process
 *			- #IOTC_ER_TIMEOUT No connection is established from clients before timeout expires
 *			- #IOTC_ER_EXIT_LISTEN The device stops listening for connections from clients.
 *
 * \attention (1) This function is a block process, waiting for following two
 *				conditions happens before executing	sequential instructions
 *				(a) any connection from clients is established (b) timeout expires.<br><br>
 *				(2) nTimeout has no effect in 8051 platform.
 */
P2PAPI_API int  IOTC_Listen(unsigned int nTimeout);


/**
 * \brief Used by a device to exit listen process
 *
 * \details Since listen process is a block process and that means a device
 *			will wait for following two conditions happens before executing
 *			sequential instructions (1) any connection from clients is established
 *			(2) timeout expires. In some cases, users may want the device to
 *			exit listen immediately by this function in another thread before
 *			the two conditions above happens.
 */
P2PAPI_API void IOTC_Listen_Exit(void);


/**
 * \brief To get a new connected session id.
 *
 * \detials As a device, it allows a client to connect. The way to know if
 *			a client connects or not is to get the device status information
 *			by calling IOTC_Get_Device_Status. Once the information indicates
 *			a new client is connected, IOTC_Accept() is used for getting the
 *			new session id for further usage.
 *
 * \param SID [out] The session id for the new connection.
 *
 * \return IOTC_ER_NoERROR on success.
 * \return The value < 0
 *			- IOTC_ER_INVALID_ARG   Invalid input argument.
 *			- IOTC_ER_NOT_INITIALIZED     The module has not bee initialized.
 *			- IOTC_ER_EXCEED_MAX_SESSION     It reaches the max session number.
 */

P2PAPI_API int  IOTC_Accept(int *SID);


/**
 * \brief Used by a client to connect a device
 *
 * \details This function is for a client to connect a device by specifying
 *			the UID of that device. If connection is established with the
 *			help of IOTC servers, the IOTC session ID will be returned in this
 *			function and then device and client can communicate for the other
 *			later by using this IOTC session ID.
 *
 * \param cszUID [in] The UID of a device that client wants to connect
 *
 * \return IOTC session ID if return value >= 0
 * \return Error code if return value < 0
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_UNLICENSE The specified UID of that device is not licensed or expired
 *			- #IOTC_ER_EXCEED_MAX_SESSION The number of IOTC sessions has reached maximum in client side
 *			- #IOTC_ER_DEVICE_NOT_LISTENING The device is not listening for connection now
 *			- #IOTC_ER_FAIL_CONNECT_SEARCH The client stop connecting to the device
 *			- #IOTC_ER_FAIL_RESOLVE_HOSTNAME Cannot resolve masters' host name
 *			- #IOTC_ER_FAIL_CREATE_THREAD Fails to create threads
 *			- #IOTC_ER_TCP_TRAVEL_FAILED Cannot connect to masters in neither UDP nor TCP
 *			- #IOTC_ER_TCP_CONNECT_TO_SERVER_FAILED Cannot connect to IOTC servers in TCP
 *			- #IOTC_ER_CAN_NOT_FIND_DEVICE IOTC servers cannot locate the specified device
 *			- #IOTC_ER_NO_PERMISSION The specified device does not support TCP relay
 *			- #IOTC_ER_SERVER_NOT_RESPONSE IOTC servers have no response
 *			- #IOTC_ER_FAIL_GET_LOCAL_IP Fails to get the client's local IP address
 *			- #IOTC_ER_FAIL_SETUP_RELAY Fails to connect the device via relay mode
 *			- #IOTC_ER_FAIL_CREATE_SOCKET Fails to create sockets
 *			- #IOTC_ER_FAIL_SOCKET_OPT Fails to set up socket options
 *			- #IOTC_ER_FAIL_SOCKET_BIND Fails to bind sockets
 *			- #IOTC_ER_NOT_SUPPORT_RELAY Not support relay connection by IOTC servers
 *			- #IOTC_ER_NO_SERVER_LIST No IOTC server information while client connect
 *			- #IOTC_ER_DEVICE_MULTI_LOGIN The connecting device duplicated loggin and may unconnectable
 *			- #IOTC_ER_MASTER_NOT_RESPONSE All masters have no respond
 *			- #IOTC_ER_DEVICE_IS_SLEEP Device is in sleep mode
 *
 * \attention This process is a block process.
 *
 */
P2PAPI_API int  IOTC_Connect_ByUID(const char *cszUID);


/**
 * \brief Used by a client to connect a device. Non-Blocking.
 *
 * \details This function is used for connecting a device by a specific UID.
 *			After this function is called, a IOTC session ID will be retruned
 *			if no error occurs. This function triggers the connecting mechanisms
 *			to connect a device. A handler should be specified for handling each
 *			state of connecting. The handler will be called once the state has been
 *			changed. The IOTC Session ID should be used to read/write the data
 *			afterwards.
 *
 *
 * \param cszUID [in] The UID of a device that client wants to connect
 * \param handler [in] The state handler. This handler will be called once
 *					   the connecting state (IOTCConnectState) is changed.
 * \param userData [in] The user data which will be brought to the handler.
 *
 * \return IOTC session ID if return value >= 0
 * \return Error code if return value < 0
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_UNLICENSE The specified UID of that device is not licensed or expired
 *			- #IOTC_ER_EXCEED_MAX_SESSION The number of IOTC sessions has reached maximum in client side
 *			- #IOTC_ER_DEVICE_NOT_LISTENING The device is not listening for connection now
 *			- #IOTC_ER_FAIL_CONNECT_SEARCH The client stop connecting to the device
 *			- #IOTC_ER_FAIL_CREATE_THREAD Fails to create threads
 *			- #IOTC_ER_CAN_NOT_FIND_DEVICE IOTC servers cannot locate the specified device
 *			- #IOTC_ER_FAIL_GET_LOCAL_IP Fails to get the client's local IP address
 *			- #IOTC_ER_FAIL_SETUP_RELAY Fails to connect the device via relay mode
 *			- #IOTC_ER_FAIL_CREATE_SOCKET Fails to create sockets
 *			- #IOTC_ER_FAIL_SOCKET_OPT Fails to set up socket options
 *			- #IOTC_ER_FAIL_SOCKET_BIND Fails to bind sockets
 *			- #IOTC_ER_NOT_SUPPORT_RELAY Not support relay connection by IOTC servers
 *
 */
P2PAPI_API int  IOTC_Connect_ByUIDNB(const char *cszUID,
                                     connectStateHandler handler,
                                     void *userData);


/**
* \brief Used by a client to get a tutk_platform_free session ID.
*
* \details This function is for a client to get a tutk_platform_free session ID used for a parameter of
*          IOTC_Connect_ByUID_Parallel().
*
* \return IOTC session ID if return value >= 0
* \return Error code if return value < 0
*			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
*			- #IOTC_ER_EXCEED_MAX_SESSION The number of IOTC sessions has reached maximum in client side
*
* \attention (1) If you already get a session ID via this function must call IOTC_Connect_ByUID_Parallel() and then
*                 it will release session ID resource automatically when connection failed.<br><br>
*            (2) If you already get a session ID via this function and want to close it please use IOTC_Session_Close().
*
*/
P2PAPI_API int  IOTC_Get_SessionID(void);


/**
 * \brief Used by a client to connect a device and bind to a specified session ID.
 *
 * \details This function is for a client to connect a device by specifying
 *			the UID of that device, and bind to a tutk_platform_free session ID from IOTC_Get_SessionID().
 * 			If connection is established with the help of IOTC servers,
 *			the #IOTC_ER_NoERROR will be returned in this function and then device and
 *			client can communicate for the other later by using this IOTC session ID.
 *			If this function is called by multiple threads, the connections will be
 *			processed concurrently.
 *
 * \param cszUID [in] The UID of a device that client wants to connect
 * \param SID [in] The Session ID got from IOTC_Get_SessionID() the connection should bind to.
 *
 * \return IOTC session ID if return value >= 0 and equal to the input parameter SID.
 * \return Error code if return value < 0
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_UNLICENSE The specified UID of that device is not licensed or expired
 *			- #IOTC_ER_EXCEED_MAX_SESSION The number of IOTC sessions has reached maximum in client side
 *			- #IOTC_ER_DEVICE_NOT_LISTENING The device is not listening for connection now
 *			- #IOTC_ER_FAIL_CONNECT_SEARCH The client stop connecting to the device
 *			- #IOTC_ER_FAIL_RESOLVE_HOSTNAME Cannot resolve masters' host name
 *			- #IOTC_ER_FAIL_CREATE_THREAD Fails to create threads
 *			- #IOTC_ER_TCP_TRAVEL_FAILED Cannot connect to masters in neither UDP nor TCP
 *			- #IOTC_ER_TCP_CONNECT_TO_SERVER_FAILED Cannot connect to IOTC servers in TCP
 *			- #IOTC_ER_CAN_NOT_FIND_DEVICE IOTC servers cannot locate the specified device
 *			- #IOTC_ER_NO_PERMISSION The specified device does not support TCP relay
 *			- #IOTC_ER_SERVER_NOT_RESPONSE IOTC servers have no response
 *			- #IOTC_ER_FAIL_GET_LOCAL_IP Fails to get the client's local IP address
 *			- #IOTC_ER_FAIL_SETUP_RELAY Fails to connect the device via relay mode
 *			- #IOTC_ER_FAIL_CREATE_SOCKET Fails to create sockets
 *			- #IOTC_ER_FAIL_SOCKET_OPT Fails to set up socket options
 *			- #IOTC_ER_FAIL_SOCKET_BIND Fails to bind sockets
 *			- #IOTC_ER_NOT_SUPPORT_RELAY Not support relay connection by IOTC servers
 *			- #IOTC_ER_NO_SERVER_LIST No IOTC server information while client connect
 *			- #IOTC_ER_DEVICE_MULTI_LOGIN The connecting device duplicated login and may unconnectable
 *			- #IOTC_ER_MASTER_NOT_RESPONSE All masters have no respond
 *			- #IOTC_ER_INVALID_SID The specified IOTC session ID is not valid
 *			- #IOTC_ER_DEVICE_IS_SLEEP Device is in sleep mode
 *
 * \attention If you call IOTC_Connect_Stop_BySID() and this function not return yet, and then use the same
 *              session ID to call again will cause session ID in wrong status.
 *
 */
P2PAPI_API int  IOTC_Connect_ByUID_Parallel(const char *cszUID, int SID);


/**
 * \brief Used by a client to connect a device and bind to a specified session ID. Non-Blocking
 *
 * \details This function is for a client to connect a device by specifying
 *			the UID of that device, and bind to a tutk_platform_free session ID from IOTC_Get_SessionID().
 * 			If connection is established with the help of IOTC servers,
 *			the #IOTC_ER_NoERROR will be returned in this function and then device and
 *			client can communicate for the other later by using this IOTC session ID.
 *			If this function is called by multiple threads, the connections will be
 *			processed concurrently. Since this function is a non-blocking call, it only
 *			triggers the connecting mechanisms to connect a device. A handler should be
 *			specified for handling each state of connecting. The handler will be called
 *			once the state has been changed.
 *
 * \param cszUID [in] The UID of a device that client wants to connect
 * \param SID [in] The Session ID got from IOTC_Get_SessionID() the connection should bind to.
 * \param handler [in] The state handler. This handler will be called once
 *					   the connecting state (IOTCConnectState) is changed.
 * \param userData [in] The user data which will be brought to the handler.
 *
 * \return IOTC session ID if return value >= 0 and equal to the input parameter SID.
 * \return Error code if return value < 0
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_UNLICENSE The specified UID of that device is not licensed or expired
 *			- #IOTC_ER_EXCEED_MAX_SESSION The number of IOTC sessions has reached maximum in client side
 *			- #IOTC_ER_DEVICE_NOT_LISTENING The device is not listening for connection now
 *			- #IOTC_ER_FAIL_CONNECT_SEARCH The client stop connecting to the device
 *			- #IOTC_ER_FAIL_RESOLVE_HOSTNAME Cannot resolve masters' host name
 *			- #IOTC_ER_FAIL_CREATE_THREAD Fails to create threads
 *			- #IOTC_ER_CAN_NOT_FIND_DEVICE IOTC servers cannot locate the specified device
 *			- #IOTC_ER_NO_PERMISSION The specified device does not support TCP relay
 *			- #IOTC_ER_FAIL_GET_LOCAL_IP Fails to get the client's local IP address
 *			- #IOTC_ER_FAIL_SETUP_RELAY Fails to connect the device via relay mode
 *			- #IOTC_ER_FAIL_CREATE_SOCKET Fails to create sockets
 *			- #IOTC_ER_FAIL_SOCKET_OPT Fails to set up socket options
 *			- #IOTC_ER_FAIL_SOCKET_BIND Fails to bind sockets
 *			- #IOTC_ER_NOT_SUPPORT_RELAY Not support relay connection by IOTC servers
 *			- #IOTC_ER_INVALID_SID The specified IOTC session ID is not valid
 *
 * \attention If you call IOTC_Connect_Stop_BySID() and this function not return yet, and then use the same
 *              session ID to call again will cause session ID in wrong status.
 *
 */
P2PAPI_API int  IOTC_Connect_ByUID_ParallelNB(const char *cszUID, int SID,
        connectStateHandler handler,
        void *userData);


/**
 * \brief Used by a client to stop connecting a device
 *
 * \details This function is for a client to stop connecting a device. Since
 *			IOTC_Connect_ByUID(), IOTC_Connect_ByUID2() are all block processes, that means
 *			the client will have to wait for the return of these functions before
 *			executing sequential instructions. In some cases, users may want
 *			the client to stop connecting immediately by this function in
 *			another thread before the return of connection process.
 *
 * \attention Only use to stop IOTC_Connect_ByUID() and 2, NOT use to stop IOTC_Connect_ByUID_Parallel().
 */
P2PAPI_API void IOTC_Connect_Stop(void);


/**
 * \brief Used by a client to stop a specific session connecting a device
 *
 * \details This function is for a client to stop connecting a device. Since
 *			IOTC_Connect_ByUID_Parallel() is a block processes, that means
 *			the client will have to wait for the return of these functions before
 *			executing sequential instructions. In some cases, users may want
 *			the client to stop connecting immediately by this function in
 *			another thread before the return of connection process.
 *
 * \param SID [in] The Session ID of a connection which will be stop.
 *
 * \return #IOTC_ER_NoERROR
 * \return Error code if return value < 0
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_INVALID_SID The specified IOTC session ID is not valid
 *			- #IOTC_ER_SESSION_CLOSE_BY_REMOTE The IOTC session of specified
 *				session ID has been closed by remote site
 *			- #IOTC_ER_REMOTE_TIMEOUT_DISCONNECT The timeout defined by #IOTC_SESSION_ALIVE_TIMEOUT
 *				expires because	remote site has no response
 */
P2PAPI_API int IOTC_Connect_Stop_BySID(int SID);


/**
 * \brief Used by a device or a client to read data from the other
 *
 * \details A device or a client uses this function to read data through
 *			a specific IOTC channel in a IOTC session. <br>
 *			The difference between this function and IOTC_Session_Read() is
 *			this function provides packet lost information. Users may use
 *			this to check how many packets, if any, have been lost since the last
 *			time reading from this session.
 *
 * \param nIOTCSessionID [in] The session ID of the IOTC session to read data
 * \param abBuf [out] The array of byte buffer to receive read result
 * \param nMaxBufSize [in] The maximum length of the byte buffer
 * \param nTimeout [in] The timeout for this function in unit of millisecond, give 0 means return immediately
 * \param pnPacketSN [out] The serial number of the packet that is read successfully
 *							this time. Could be NULL.
 * \param pbFlagLost [out] A boolean value to indicate if there are some packets
 *							lost between this time and previous successful read.
 *							Could be NULL.
 * \param nIOTCChannelID [in] The IOTC channel ID in this IOTC session to read data
 *
 * \return The actual length of read result stored in abBuf if read successfully
 * \return Error code if return value < 0
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_INVALID_SID The specified IOTC session ID is not valid
 *			- #IOTC_ER_CH_NOT_ON The IOTC channel of specified channel ID is not turned on
 *			- #IOTC_ER_SESSION_CLOSE_BY_REMOTE The IOTC session of specified
 *				session ID has been closed by remote site
 *			- #IOTC_ER_REMOTE_TIMEOUT_DISCONNECT The timeout defined by #IOTC_SESSION_ALIVE_TIMEOUT
 *				expires because	remote site has no response
 *			- #IOTC_ER_TIMEOUT The timeout specified by nTimeout expires before
 *				read process is performed completely
 *
 * \attention The IOTC channel of ID 0 is enabled by default when a IOTC session is established.
 *				That means nIOTCChannelID can be specified as 0 if only one IOTC channel
 *				is needed by the user. If more IOTC channels are required, users
 *				should use IOTC_Session_Get_Free_Channel() and IOTC_Session_Channel_ON()
 *				to get more IOTC channel IDs and then specifying those IOTC channel IDs
 *				in this function according to the purpose defined by the user.
 */
P2PAPI_API int  IOTC_Session_Read_Check_Lost(int nIOTCSessionID, char *abBuf, int nMaxBufSize, unsigned int nTimeout, unsigned short *pnPacketSN, char *pbFlagLost, unsigned char nIOTCChannelID);


/**
 * \brief Used by a device or a client to check the IOTC session info
 *
 * \details A device or a client may use this function to check if the IOTC session
 *			is still alive as well as getting the IOTC session info.
 *
 * \param nIOTCSessionID [in] The session ID of the IOTC session to be checked
 * \param psSessionInfo [out] The session info of specified IOTC session
 *
 * \return #IOTC_ER_NoERROR if getting the IOTC session info successfully
 * \return Error code if return value < 0
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_INVALID_SID The specified IOTC session ID is not valid
 *			- #IOTC_ER_SESSION_CLOSE_BY_REMOTE The IOTC session of specified
 *				session ID has been closed by remote site
 *			- #IOTC_ER_REMOTE_TIMEOUT_DISCONNECT The timeout defined by #IOTC_SESSION_ALIVE_TIMEOUT
 *				expires because	remote site has no response
 *			- #IOTC_ER_INVALID_ARG The arguments passed in to this function is invalid.
 */
P2PAPI_API int  IOTC_Session_Check(int nIOTCSessionID, struct st_SInfo *psSessionInfo);


/**
 * \brief Used by a device or a client to check the IOTC session info
 *
 * \details A device or a client may use this function to check if the IOTC session
 *			is still alive as well as getting the IOTC session info. The structure
 *          size must be filled. For example "info.size=sizeof(struct st_SInfoEx);"
 *
 * \param nIOTCSessionID [in] The session ID of the IOTC session to be checked
 * \param psSessionInfo [out] The session info of specified IOTC session
 *
 * \return #IOTC_ER_NoERROR if getting the IOTC session info successfully
 * \return Error code if return value < 0
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_INVALID_SID The specified IOTC session ID is not valid
 *			- #IOTC_ER_SESSION_CLOSE_BY_REMOTE The IOTC session of specified
 *				session ID has been closed by remote site
 *			- #IOTC_ER_REMOTE_TIMEOUT_DISCONNECT The timeout defined by #IOTC_SESSION_ALIVE_TIMEOUT
 *				expires because	remote site has no response
 *          - #IOTC_ER_INVALID_ARG The size of structure is not filled
 */
P2PAPI_API int  IOTC_Session_Check_Ex(int nIOTCSessionID, struct st_SInfoEx *psSessionInfo);


/**
 * \brief Used by a device or a client to get the session status
 *
 * \details This function gets the session status between device and client. <br>
 *			The difference of this function and IOTC_Session_Check() is
 *			this function will set callback function inside IOTC module and
 *			that callback function will be invoked whenever the session status
 *			between device and client disconnected, for example, IOTC
 *			alive timeout or one side call IOTC_Session_Close() to close
 *			this session.

 * \param nIOTCSessionID [in] The session ID of the IOTC session to check status
 * \param pfxSessionStatusFn [in] The function pointer to getting session status function
 *
 * \return #IOTC_ER_NoERROR if getting the IOTC session info successfully
 * \return Error code if return value < 0
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_INVALID_SID The specified IOTC session ID is not valid
 *			- #IOTC_ER_SESSION_CLOSE_BY_REMOTE The IOTC session of specified
 *				session ID has been closed by remote site
 *			- #IOTC_ER_REMOTE_TIMEOUT_DISCONNECT The timeout defined by #IOTC_SESSION_ALIVE_TIMEOUT
 *				expires because	remote site has no response
 */
P2PAPI_API int IOTC_Session_Check_ByCallBackFn(int nIOTCSessionID, sessionStatusCB pfxSessionStatusFn);


/**
 * \brief Used by a device or a client to read data from the other
 *
 * \details A device or a client uses this function to read data through
 *			a specific IOTC channel in a IOTC session.
 *
 * \param nIOTCSessionID [in] The session ID of the IOTC session to read data
 * \param abBuf [out] The array of byte buffer to receive read result
 * \param nMaxBufSize [in] The maximum length of the byte buffer
 * \param nTimeout [in] The timeout for this function in unit of millisecond, give 0 means return immediately
 * \param nIOTCChannelID [in] The IOTC channel ID in this IOTC session to read data
 *
 * \return The actual length of read result stored in abBuf if read successfully
 * \return Error code if return value < 0
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_INVALID_SID The specified IOTC session ID is not valid
 *			- #IOTC_ER_CH_NOT_ON The IOTC channel of specified channel ID is not turned on
 *			- #IOTC_ER_SESSION_CLOSE_BY_REMOTE The IOTC session of specified
 *				session ID has been closed by remote site
 *			- #IOTC_ER_REMOTE_TIMEOUT_DISCONNECT The timeout defined by #IOTC_SESSION_ALIVE_TIMEOUT
 *				expires because	remote site has no response
 *			- #IOTC_ER_TIMEOUT The timeout specified by nTimeout expires before
 *				read process is performed completely
 *
 * \attention (1) The IOTC channel of ID 0 is enabled by default when a IOTC session is established.
 *				That means nIOTCChannelID can be specified as 0 if only one IOTC channel
 *				is needed by the user. If more IOTC channels are required, users
 *				should use IOTC_Session_Get_Free_Channel() and IOTC_Session_Channel_ON()
 *				to get more IOTC channel IDs and then specifying those IOTC channel IDs
 *				in this function according to the purpose defined by the user.<br><br>
 *				(2) If the size of abBuf, i.e. defined by nMaxBufSize, is less than
 *				the size of data to be read, then this function will only read
 *				the data up to nMaxBufSize and the remaining part will be truncated without
 *				error code returned. Therefore, it is suggested to allocate the size
 *				of abBuf as #IOTC_MAX_PACKET_SIZE for ensure complete reading.
 *				(3) This function will need 192 bytes stack size.
 *
 */
P2PAPI_API int  IOTC_Session_Read(int nIOTCSessionID, char *abBuf, int nMaxBufSize, unsigned int nTimeout, unsigned char nIOTCChannelID);


/**
 * \brief Used by a device or a client to write data to the other
 *
 * \details A device or a client uses this function to write data through
 *			a specific IOTC channel in a IOTC session.
 *
 * \param nIOTCSessionID [in] The session ID of the IOTC session to write data
 * \param cabBuf [in] The array of byte buffer containing the data to write.
 *			Its size cannot be larger than #IOTC_MAX_PACKET_SIZE
 * \param nBufSize [in] The length of the byte buffer. It cannot be larger than
 *			#IOTC_MAX_PACKET_SIZE
 * \param nIOTCChannelID [in] The IOTC channel ID in this IOTC session to write data
 *
 * \return The actual length of buffer to be written if write successfully. In non-blocking
 *         mode, the length with zero usually means the socket buffer is full and unable to
 *         write into.
 * \return Error code if return value < 0
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_INVALID_SID The specified IOTC session ID is not valid
 *			- #IOTC_ER_CH_NOT_ON The IOTC channel of specified channel ID is not turned on
 *			- #IOTC_ER_SESSION_CLOSE_BY_REMOTE The IOTC session of specified
 *				session ID has been closed by remote site
 *			- #IOTC_ER_REMOTE_TIMEOUT_DISCONNECT The timeout defined by #IOTC_SESSION_ALIVE_TIMEOUT
 *				expires because	remote site has no response
 *			- #IOTC_ER_INVALID_ARG The buffer size is not accepted
 *			- #IOTC_ER_NO_PATH_TO_WRITE_DATA IOTC internal error, cannot find a path to send data
 *
 * \attention (1) The IOTC channel of ID 0 is enabled by default when a IOTC session is established.
 *				That means nIOTCChannelID can be specified as 0 if only one IOTC channel
 *				is needed by the user. If more IOTC channels are required, users
 *				should use IOTC_Session_Get_Free_Channel() and IOTC_Session_Channel_ON()
 *				to get more IOTC channel IDs and then specifying those IOTC channel IDs
 *				in this function according to the purpose defined by the user. <br><br>
 *				(2) This function will block when session is connected via TCP and socket buffer is full.
 *				(3) This function will need 4976 bytes stack size.
 *
 */
P2PAPI_API int IOTC_Session_Write(int nIOTCSessionID, const char *cabBuf, int nBufSize, unsigned char nIOTCChannelID);


/**
 * \brief Used by a device or a client to close a IOTC session
 *
 * \details A device or a client uses this function to close a IOTC session
 *			specified by its session ID if this IOTC session is no longer
 *			required. If the closed SID is used for other APIs, the API should
 *			return IOTC_ER_INVALID_SID.
 *
 * \param nIOTCSessionID [in] The session ID of the IOTC session to be closed
 *
 */
P2PAPI_API void IOTC_Session_Close(int nIOTCSessionID);


/**
 * \brief Used by a device or a client to get a tutk_platform_free IOTC channel
 *
 * \details A device or a client uses this function to get a tutk_platform_free IOTC channel
 *			in a specified IOTC session. By default, IOTC channel of ID 0 is turned on
 *			once a IOTC session is established. If more IOTC channels are required
 *			by users, this function can always return a tutk_platform_free IOTC channel until
 *			maximum IOTC channels are reached.
 *
 * \param nIOTCSessionID [in] The session ID of the IOTC session to get tutk_platform_free IOTC channel
 *
 * \return The IOTC channel ID of a tutk_platform_free IOTC channel if successfully
 * \return Error code if return value < 0
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_INVALID_SID The specified IOTC session ID is not valid
 *			- #IOTC_ER_SESSION_CLOSE_BY_REMOTE The IOTC session of specified
 *				session ID has been closed by remote site
 *			- #IOTC_ER_REMOTE_TIMEOUT_DISCONNECT The timeout defined by #IOTC_SESSION_ALIVE_TIMEOUT
 *				expires because	remote site has no response
 *			- #IOTC_ER_SESSION_NO_FREE_CHANNEL Already reach the maximum
 *				number of IOTC channels, no more tutk_platform_free IOTC channel is available
 *
 * \attention (1) The IOTC channel returned by this function is already turned on.<br><br>
 *				(2) The IOTC channel is only turned on in the local site
 *				calling	this function. That means, the remote site shall use
 *				IOTC_Session_Channel_ON() to turn on the same IOTC channel at its
 *				side before communication.
 */
P2PAPI_API int IOTC_Session_Get_Free_Channel(int nIOTCSessionID);


/**
 * \brief Used by a device or a client to turn on a IOTC channel
 *
 * \details A device or a client uses this function to turn on a IOTC channel
 *			before sending or receiving data through this IOTC channel.
 *
 * \param nIOTCSessionID [in] The session ID of the IOTC session containing the
 *			IOTC channel to be turned on
 * \param nIOTCChannelID [in] The channel ID of the IOTC channel to be turned on
 *
 * \return IOTC_ER_NoERROR if turning on the IOTC channel successfully
 * \return Error code if return value < 0
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_INVALID_SID The specified IOTC session ID is not valid
 *			- #IOTC_ER_SESSION_CLOSE_BY_REMOTE The IOTC session of specified
 *				session ID has been closed by remote site
 *			- #IOTC_ER_REMOTE_TIMEOUT_DISCONNECT The timeout defined by #IOTC_SESSION_ALIVE_TIMEOUT
 *				expires because	remote site has no response
 *			- #IOTC_ER_CH_NOT_ON The specified IOTC channel ID is not valid
 *
 * \attention The IOTC channel is only turned on in the local site calling
 *				this function. That means, the remote site shall also use
 *				IOTC_Session_Channel_ON() to turn on the same IOTC channel
 *				at its side	before communication.
 */
P2PAPI_API int IOTC_Session_Channel_ON(int nIOTCSessionID, unsigned char nIOTCChannelID);


/**
 * \brief Used by a device or a client to turn off a IOTC channel
 *
 * \details A device or a client uses this function to turn off a IOTC channel
 *			when this IOTC channel is no longer needed for communication.
 *
 * \param nIOTCSessionID [in] The session ID of the IOTC session containing the
 *			IOTC channel to be turned off
 * \param nIOTCChannelID [in] The channel ID of the IOTC channel to be turned off
 *
 * \return IOTC_ER_NoERROR if turning off the IOTC channel successfully
 * \return Error code if return value < 0
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_INVALID_SID The specified IOTC session ID is not valid
 *			- #IOTC_ER_SESSION_CLOSE_BY_REMOTE The IOTC session of specified
 *				session ID has been closed by remote site
 *			- #IOTC_ER_REMOTE_TIMEOUT_DISCONNECT The timeout defined by #IOTC_SESSION_ALIVE_TIMEOUT
 *				expires because	remote site has no response
 *			- #IOTC_ER_CH_NOT_ON The specified IOTC channel ID is not valid
 *
 * \attention (1) IOTC Channel 0 cannot be turned off because it is a default channel.<br><br>
 *				(2) Turn off a IOTC channel will also make the data remaining
 *				in receiving queue of this channel be deleted.
 */
P2PAPI_API int IOTC_Session_Channel_OFF(int nIOTCSessionID, unsigned char nIOTCChannelID);


/**
 * \brief Used by a device or a client to check whether the IOTC channel is ON or OFF
 *
 * \details A device or a client uses this function to turn on a IOTC channel
 *          before sending or receiving data through this IOTC channel.
 *
 * \param nIOTCSessionID [in] The session ID of the IOTC session containing the
 *          IOTC channel to be checked
 * \param nIOTCChannelID [in] The channel ID of the IOTC channel to be checked
 *
 * \return 1 for channel ON return 0 for channel OFF
 * \return Error code if return value < 0
 *          - #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *          - #IOTC_ER_INVALID_SID The specified IOTC session ID is not valid
 *          - #IOTC_ER_SESSION_CLOSE_BY_REMOTE The IOTC session of specified
 *              session ID has been closed by remote site
 *          - #IOTC_ER_REMOTE_TIMEOUT_DISCONNECT The timeout defined by #IOTC_SESSION_ALIVE_TIMEOUT
 *              expires because remote site has no response
 *          - #IOTC_ER_INVALID_ARG The specified IOTC channel ID is not valid
 *
 */
P2PAPI_API int IOTC_Session_Channel_Check_ON_OFF(int nIOTCSessionID, unsigned char nIOTCChannelID);


/**
 * \brief Used by search devices in LAN.
 *
 * \details When clients and devices are stay in a LAN environment, client can call this function
 *			to discovery devices and connect it directly.
 *
 * \param psLanSearchInfo [in] The array of struct st_LanSearchInfo to store search result
 *
 * \param nArrayLen [in] The size of the psLanSearchInfo array
 *
 * \param nWaitTimeMs [in] The timeout in milliseconds before discovery process end.
 *
 * \return The number of devices found.
 * \return Error code if return value < 0
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_INVALID_ARG The arguments passed in to this function is invalid.
 *			- #IOTC_ER_FAIL_CREATE_SOCKET Fails to create sockets
 *			- #IOTC_ER_FAIL_SOCKET_OPT Fails to set up socket options
 *			- #IOTC_ER_FAIL_SOCKET_BIND Fails to bind sockets
 *
 * \attention (1) Not support IPv6. The IP length is only for IPv4.<br><br>*
 * 			  (2) This API is deprecated. Please use IOTC_Search_Device_Start / IOTC_Search_Device_Result.<br><br>*
 */
P2PAPI_API int IOTC_Lan_Search(struct st_LanSearchInfo *psLanSearchInfo, int nArrayLen, int nWaitTimeMs);


/**
 * \brief Set path of log file
 *
 * \details Set the absolute path of log file
 *
 * \param path [in] The path of log file, NULL = disable Log
 *
 * \param nMaxSize [in] The maximum size of log file in Bytes, 0 = unlimited
 *
 */
P2PAPI_API	void IOTC_Set_Log_Path(char *path, int nMaxSize);


/**
 * \brief Set partial encode On/Off
 *
 * \details Set partial encode On/Off
 *
 * \param nIOTCSessionID [in] The session ID of the IOTC session to be encrypted.
 *
 * \param bPartialEncryption [in] 1: Enable partial encode, 0: Disable partial encode
 *
 * \return Error code if return value < 0
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_INVALID_SID The specified IOTC session ID is not valid
 *			- #IOTC_ER_SESSION_CLOSE_BY_REMOTE The IOTC session of specified
 *				session ID has been closed by remote site
 *			- #IOTC_ER_REMOTE_TIMEOUT_DISCONNECT The timeout defined by #IOTC_SESSION_ALIVE_TIMEOUT
 *				expires because	remote site has no response
 *			- #IOTC_ER_NOT_SUPPORT_PE The remote device don't support partial encryption
 *
 */
P2PAPI_API	int IOTC_Set_Partial_Encryption(int nIOTCSessionID, unsigned char bPartialEncryption);


/**
 * \brief Set device name.
 *
 * \details Device can let client know its name when client call IOTC_Lan_Search2().
 *          The maximum size of device name is 131 Byte. We filled in 0 at the 132th Byte.
 *
 * \param cszDeviceName  [in] This is user-defined device name. Clients will get it by calling IOTC_Lan_Search2().
 *
 */
P2PAPI_API  void IOTC_Set_Device_Name(const char *cszDeviceName);


/**
 * \brief Used for searching devices in LAN.
 *
 * \details When client and devices are in LAN, client can search devices and their name
 *			by calling this function.
 *
 * \param psLanSearchInfo2 [in] The array of struct st_LanSearchInfo2 store the search result and Device name.
 *
 * \param nArrayLen [in] The size of psLanSearchInfo2 array
 *
 * \param nWaitTimeMs [in] Period (or timeout) of searching LAN. (milliseconds)
 *
 * \return The number of devices found.
 * \return Error code if return value < 0
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_INVALID_ARG The arguments passed in to this function is invalid.
 *			- #IOTC_ER_FAIL_CREATE_SOCKET Fails to create sockets
 *			- #IOTC_ER_FAIL_SOCKET_OPT Fails to set up socket options
 *			- #IOTC_ER_FAIL_SOCKET_BIND Fails to bind sockets
 *
 * \attention (1) Not support IPv6. The IP length is only for IPv4.<br><br>*
 * 			  (2) This API is deprecated. Please use IOTC_Search_Device_Start / IOTC_Search_Device_Result.<br><br>*
 */
P2PAPI_API  int IOTC_Lan_Search2(struct st_LanSearchInfo2 *psLanSearchInfo2, int nArrayLen, int nWaitTimeMs);


/**
 * \brief Used for searching devices in LAN.
 *
 * \details When client and devices are in LAN, client can search devices and their name
 *			by calling this function.
 *
 * \param psLanSearchInfo2 [in] The array of struct st_LanSearchInfo2 store the search result and Device name.
 * \param nArrayLen [in] The size of psLanSearchInfo2 array
 * \param nWaitTimeMs [in] Period (or timeout) of searching LAN. (milliseconds)
 * \param nSendIntervalMs [in] Interval of sending broadcast for searching device in LAN. (milliseconds)
 *
 * \return The number of devices found.
 * \return Error code if return value < 0
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_INVALID_ARG The arguments passed in to this function is invalid.
 *			- #IOTC_ER_FAIL_CREATE_SOCKET Fails to create sockets
 *			- #IOTC_ER_FAIL_SOCKET_OPT Fails to set up socket options
 *			- #IOTC_ER_FAIL_SOCKET_BIND Fails to bind sockets
 *
 * \attention (1) Not support IPv6. The IP length is only for IPv4.<br><br>*
 * 			  (2) This API is deprecated. Please use IOTC_Search_Device_Start / IOTC_Search_Device_Result.<br><br>*
 */
P2PAPI_API int IOTC_Lan_Search2_Ex(struct st_LanSearchInfo2 *psLanSearchInfo2, int nArrayLen, int nWaitTimeMs, int nSendIntervalMs);


/**
* \brief Start to search devices in LAN.
*
* \details When client and devices are in LAN, client can search devices and their name
*			and the result can be polled by function IOTC_Search_Device_Result
*
* \param nWaitTimeMs [in] Period (or timeout) of searching LAN. (milliseconds)
* \param nSendIntervalMs [in] Interval of sending broadcast for searching device in LAN. (milliseconds)
*
* \return The number of devices found in the LAN (it is limited by the length of array)
* \return Error code if return value < 0
*			- #IOTC_ER_INVALID_ARG The arguments passed in to this function is invalid.
*			- #IOTC_ER_FAIL_CREATE_SOCKET Fails to create sockets
*			- #IOTC_ER_FAIL_SOCKET_OPT Fails to set up socket options
*			- #IOTC_ER_FAIL_SOCKET_BIND Fails to bind sockets
*			- #IOTC_ER_STILL_IN_PROCESSING The function is called more then once before it stopped
*			- #IOTC_ER_NOT_ENOUGH_MEMORY not enough memory
*/
P2PAPI_API int IOTC_Search_Device_Start(int nWaitTimeMs, int nSendIntervalMs);


/**
* \brief Poll the results of searched device in LAN.
*
* \details Use the function to poll the result of device search in LAN, the IOTC_Search_Device_Start should be called
*			before use the function
*
* \param st_SearchDeviceInfo [in] The array of struct st_SearchDeviceInfo store the search result and Device name.
* \param nArrayLen [in] The length of array
* \param nGetAll [in] 0: get new queried device 1: get all queried devices
*
* \return IOTC_ER_NoERROR if search devices in LAN successfully
* \return Error code if return value < 0
*			- #IOTC_ER_INVALID_ARG The arguments passed in to this function is invalid.
*			- #IOTC_ER_SERVICE_IS_NOT_STARTED The start function is not called
*
* \attention (1) Support IPv6. The IP length supports both for IPv4/IPv6.<br><br>*
*/
P2PAPI_API int IOTC_Search_Device_Result(struct st_SearchDeviceInfo *psSearchDeviceInfo, int nArrayLen, int nGetAll);


/**
* \brief Stop to search devices in LAN.
*
* \details Stop to do device search in LAN, the IOTC_Search_Device_Start should be called
*			before use the function
*
* \param st_SearchDeviceInfo [in] The array of struct st_SearchDeviceInfo store the search result and Device name.
* \param nArrayLen [in] The length of array
* \param nGetAll [in] 0: get new queried device 1: get all queried devices
*
* \return IOTC_ER_NoERROR if search devices in LAN successfully
* \return Error code if return value < 0
*			- #IOTC_ER_SERVICE_IS_NOT_STARTED The start function is not called
*/
P2PAPI_API int IOTC_Search_Device_Stop();


/**
 * \brief Turn on only TCP relay mode function.
 *
 * \details When device call this will login via TCP and only can be connected via LAN or TCP relay mode.
 *			When client call this will connect to device only via LAN or TCP relay mode.
 *
 * \attention	Can be called anywhere.
 *
 */
P2PAPI_API void IOTC_TCPRelayOnly_TurnOn(void);


/**
 * \brief Setup LAN search and LAN connection timeout
 *
 * \details Only client can call this, it can determine how many time to try LAN search and LAN connection.
 *			Once it called the timeout value is effective forever until IOTC_DeInitialize() be called.
 *
 * \param nTimeout [in] The timeout for this function in unit of millisecond, give 0 means skip LAN flow
 *
 * \attention	Mast be called before start connection. Minimum is 100 millisecond.
 *
 */
P2PAPI_API void IOTC_Setup_LANConnection_Timeout(unsigned int nTimeout);


/**
* \brief Setup P2P connection timeout
*
* \details Only client can call this, it can determine how many time to try P2P connection.
*			Once it called the timeout value is effective forever until IOTC_DeInitialize() be called.
*
* \param nTimeout [in] The timeout for this function in unit of millisecond, give 0 means skip P2P flow
*
* \attention	Mast be called before start connection. Minimum is 100 millisecond.
*
*/
P2PAPI_API void IOTC_Setup_P2PConnection_Timeout(unsigned int nTimeout);

/**
 * \brief ReInitialize IOTC module socket
 *
 * \details This function will reopen P2P UDP socket of IOTC module.
 *			It is useful for iOS APP from backgroud to foreground will cause socket failed.
 *
 * \param nUDPPort [in] Specify a UDP port. Random UDP port is used if it is specified as 0.
 *
 * \return #IOTC_ER_NoERROR if initializing successfully
 * \return Error code if return value < 0
 *			- #IOTC_ER_FAIL_CREATE_SOCKET Fails to create sockets
 *			- #IOTC_ER_FAIL_SOCKET_OPT Fails to set up socket options
 *			- #IOTC_ER_FAIL_SOCKET_BIND Fails to bind sockets
 *
 *
 * \attention   Must call it before any connection process.
 *
 */
P2PAPI_API int IOTC_ReInitSocket(unsigned short nUDPPort);

/**
 * \brief Setup detect network reachable timeout
 *
 * \details Both device and client can call this, it can determine how many time to detect internet reachable.
 *			Once it called the timeout value is effective forever until IOTC_DeInitialize() be called.
 *
 * \param nTimeout [in] The timeout for this function in unit of millisecond
 *
 * \attention	Mast be called before start login or connection.
 *
 */
P2PAPI_API void IOTC_Setup_DetectNetwork_Timeout(unsigned int nTimeout);


/**
* \brief Used by a device or a client to write data to the other in reliable way
*
* \details A device or a client uses this function to write data through
*			a specific IOTC channel in a IOTC session in a reliable way.
*
*
* \param nIOTCSessionID [in] The session ID of the IOTC session to write data
* \param cabBuf [in] The array of byte buffer containing the data to write.
*			Its size cannot be larger than #RELIABLE_MAX_PACKET_SIZE
* \param nBufSize [in] The length of the byte buffer. It cannot be larger than
*			#RELIABLE_MAX_PACKET_SIZE
* \param nIOTCChannelID [in] The IOTC channel ID in this IOTC session to write data
*
* \return Return 0 if send successfully.
* \return Error code if return value < 0
*			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
*			- #IOTC_ER_INVALID_SID The specified IOTC session ID is not valid. OR the session has been closed.
*			- #IOTC_ER_CH_NOT_ON The IOTC channel of specified channel ID is not turned on
*			- #IOTC_ER_SESSION_CLOSE_BY_REMOTE The IOTC session of specified
*				session ID has been closed by remote site
*			- #IOTC_ER_EXCEED_MAX_PACKET_SIZE The packet size is larger than RELIABLE_MAX_PACKET_SIZE
*			- #IOTC_ER_ABORTED Was aborted by IOTC_Session_Write_Reliable_Abort().
*			- #IOTC_ER_SERVER_NOT_SUPPORT Server does not support this feature.
*			- #IOTC_ER_QUEUE_FULL The send queue is full.
*
* \attention (1) The IOTC channel of ID 0 is enabled by default when a IOTC session is established.
*				That means nIOTCChannelID can be specified as 0 if only one IOTC channel
*				is needed by the user. If more IOTC channels are required, users
*				should use IOTC_Session_Get_Free_Channel() and IOTC_Session_Channel_ON()
*				to get more IOTC channel IDs and then specifying those IOTC channel IDs
*				in this function according to the purpose defined by the user. <br><br>
*
* \attention (2) IOTC reliabe non-blocking call has a send queue. Before calling IOTC_Session_Close()
*				you might need to make sure all the messages are sent out successfully.
*				IOTC_Reliable_All_MSG_Is_Sent() is the function to check the send queue.
*				In case of the enqueue messages don't matter. IOTC_Session_Close() can be
*				calling without checking.
*
*/
P2PAPI_API int IOTC_Session_Write_Reliable_NB(int nIOTCSessionID, const char *cabBuf, int nBufSize, unsigned char nIOTCChannelID);


/**
* \brief Used by a device or a client to abort specific IOTC_Session_Write_Reliable_NB() immediately.
*
* \details A device or a client uses this function to write data through
*			a specific IOTC channel in a IOTC session in a reliable way.
*          Once this function is called, the items in the send queue will not
*          be sent out and be clear. The make queue free for writing other data.
*
* \param nIOTCSessionID [in] The session ID of the IOTC session to write data
* \param nIOTCChannelID [in] The IOTC channel ID in this IOTC session to write data
*
* \return Return 0 if IOTC_Session_Write_Reliable_Abort() call successfully.
* \return Error code if return value < 0
*			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
*			- #IOTC_ER_INVALID_SID The specified IOTC session ID is not valid
*			- #IOTC_ER_CH_NOT_ON The IOTC channel of specified channel ID is not turned on
* \attention The IOTC channel of ID 0 is enabled by default when a IOTC session is established.
*				That means nIOTCChannelID can be specified as 0 if only one IOTC channel
*				is needed by the user. If more IOTC channels are required, users
*				should use IOTC_Session_Get_Free_Channel() and IOTC_Session_Channel_ON()
*				to get more IOTC channel IDs and then specifying those IOTC channel IDs
*				in this function according to the purpose defined by the user. <br><br>
*
**/
P2PAPI_API int IOTC_Session_Write_Reliable_NB_Abort(int nIOTCSessionID, unsigned char nIOTCChannelID);


/**
* \brief Used by a device or a client to check if all the message are sent out
*		 from the reliable send queue.
*
* \details A device or a client uses this function to verify wether or not
*		   all the message are sent out succefully.
*
* \param nIOTCSessionID [in] The session ID of the IOTC session to check the data
* \param nIOTCChannelID [in] The IOTC channel ID in this IOTC session to check the data
*
* \return Return IOTC_RELIABLE_MSG_SENDING means that the queue has messages still.
*				 IOTC_RELIABLE_MSG_SEND_DONE means that all the message are sent out successfully.
*
* \return Error code if return value < 0
*			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
*			- #IOTC_ER_INVALID_SID The specified IOTC session ID is not valid
*			- #IOTC_ER_CH_NOT_ON The IOTC channel of specified channel ID is not turned on
**/
P2PAPI_API int IOTC_Reliable_All_MSG_Is_Sent(int nIOTCSessionID, unsigned char nIOTCChannelID);


/**
* \brief Setup session alive timeout value
*
* \details the default alive timeout value is 60 seconds since version 1.14.20.0,
*			this function could help you to change the alive timeout value
*
* \param nTimeout [in] The timeout for this function in unit of second
*
* \attention Must be set on client and device with the same value. Please use the function before
*				IOTC_initialize or IOTC_initialize2. The minimum value is 1.
*
*/
P2PAPI_API void IOTC_Setup_Session_Alive_Timeout(unsigned int nTimeout);


/**
* \brief Setup connect option when client connects to device.
*
* \details Client uses this function to set the Option of Connection.
*
* \param nIOTCSessionID [in] The session ID of the IOTC session to be checked.
* \param S_ConnectOption [in] the connect option that contained the option to be set.
**/
P2PAPI_API int IOTC_Set_Connection_Option(struct st_ConnectOption *S_ConnectOption);


/**
* \brief Switch of Uploading Error code to Server.
*
* \details Use this function to enable or disable error code uploading.
*           The default setup is disable.
*
* \param nEnable [in] Set nEnable to 0 to disable error code uploading.
*           Set nEnable to 1 to enable error code uploading.
**/
P2PAPI_API void IOTC_Setup_ErrorUpload(int nEnable);


/**
* \brief Register IOTC connection mode change callback.
*
* \details Callback will be triggered when connection mode is changed.
*
* \param fpCallback [in] function pointer of callback.
*
**/
P2PAPI_API void IOTC_ConnModeChange_CallBack(ConnectModeChangeCB fpCallback);


/**
* \brief Change default LAN search port
*
* \details Both device and client must have the same LAN search port, otherwise client cannot find
*		   device by IOTC_Device_Search_Start or IOTC_LAN_Search
*
* \param nPort [in] Pass the customized LAN search port
*
* \attention The IOTC_Set_LanSearchPort must be called before IOTC_Initialize or IOTC_Initialize2
*
* \return Return IOTC_ER_NoERROR if successfully.
* \return Error code if return value < 0
*			- #IOTC_ER_INVALID_ARG	The arguments passed in to this function is invalid or port number > 65535.
**/
P2PAPI_API int IOTC_Set_LanSearchPort(unsigned int nPort);


/**
 * \brief Add Customize Master server
 *
 * \details IOTC module has default master servers. If users need to add their own
 * 			master server, this function can add the customize master servers.
 *
 * \param cszMasterHostName1 [in] Specify the domain name or IP address of
 *			the primary master. Cannot be NULL. See attention below for more detail.
 * \param cszMasterHostName2 [in] Specify the domain name or IP address of
 *			the secondary master. Can be NULL if only one master is required.
 * \param cszMasterHostName3 [in] Specify the domain name or IP address of
 *			the third master. Can be NULL if only one master is required.
 * \param cszMasterHostName4 [in] Specify the domain name or IP address of
 *			the fourth master. Can be NULL if only one master is required.
 *
 * \return #IOTC_ER_NoERROR if initializing successfully
 * \return Error code if return value < 0
 *			- #IOTC_ER_NOT_INITIALIZED The IOTC module is not initialized yet
 *			- #IOTC_ER_MASTER_INVALID cszMasterDomain1 is set NULL
 *
 * \attention This function must be used before IOTC_Device_Login(), IOTC_Connect_ByUID(),
 * 			  IOTC_Connect_ByUID_Parallel() <br><br>
 */
P2PAPI_API int IOTC_Add_MasterServer(const char *cszMasterHostName1, const char *cszMasterHostName2,
                                     const char *cszMasterHostName3, const char *cszMasterHostName4);
/*
P2PAPI_API int IOTC_Add_MasterServer_Test(const char* cszMasterHostName1, const char* cszMasterHostName2,
							   		 	  const char* cszMasterHostName3, const char* cszMasterHostName4);
*/




#ifdef IOTC_UCOSII
//void *malloc(unsigned int size);
void tutk_platform_free(void *addr);
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* _IOTCAPIs_H_ */


// license:BSD-3-Clause
// copyright-holders:Samuele Zannoli

#pragma once

#include <forward_list>

struct OHCIEndpointDescriptor {
	int mps; // MaximumPacketSize
	int f; // Format
	int k; // sKip
	int s; // Speed
	int d; // Direction
	int en; // EndpointNumber
	int fa; // FunctionAddress
	UINT32 tailp; // TDQueueTailPointer
	UINT32 headp; // TDQueueHeadPointer
	UINT32 nexted; // NextED
	int c; // toggleCarry
	int h; // Halted
	UINT32 word0;
};

struct OHCITransferDescriptor {
	int cc; // ConditionCode
	int ec; // ErrorCount
	int t; // DataToggle
	int di; // DelayInterrupt
	int dp; // Direction/PID
	int r; // bufferRounding
	UINT32 cbp; // CurrentBufferPointer
	UINT32 nexttd; // NextTD
	UINT32 be; // BufferEnd
	UINT32 word0;
};

struct OHCIIsochronousTransferDescriptor {
	int cc; // ConditionCode
	int fc; // FrameCount
	int di; // DelayInterrupt
	int sf; // StartingFrame
	UINT32 bp0; // BufferPage0
	UINT32 nexttd; // NextTD
	UINT32 be; // BufferEnd
	UINT32 offset[8]; // Offset/PacketStatusWord
	UINT32 word0;
	UINT32 word1;
};

enum OHCIRegisters {
	HcRevision=0,
	HcControl,
	HcCommandStatus,
	HcInterruptStatus,
	HcInterruptEnable,
	HcInterruptDisable,
	HcHCCA,
	HcPeriodCurrentED,
	HcControlHeadED,
	HcControlCurrentED,
	HcBulkHeadED,
	HcBulkCurrentED,
	HcDoneHead,
	HcFmInterval,
	HcFmRemaining,
	HcFmNumber,
	HcPeriodicStart,
	HcLSThreshold,
	HcRhDescriptorA,
	HcRhDescriptorB,
	HcRhStatus,
	HcRhPortStatus1
};

enum HcControlBits
{
	CBSR = 3 << 0, // ControlBulkServiceRatio
	PLE = 1 << 2, // PeriodicListEnable
	IE = 1 << 3, // IsochronousEnable
	CLE = 1 << 4, // ControlListEnable
	BLE = 1 << 5, // BulkListEnable
	HCFS = 3 << 6, // HostControllerFunctionalState
	IR = 1 << 8, // InterruptRouting
	RWC = 1 << 9, // RemoteWakeupConnected
	RWE = 1 << 10 // RemoteWakeupEnable
};

enum HcCommandStatusBits
{
	HCR = 1 << 0, // HostControllerReset
	CLF = 1 << 1, // ControlListFilled
	BLF = 1 << 2, // BulkListFilled
	OCR = 1 << 3, // OwnershipChangeRequest
	SOC = 3 << 16 // SchedulingOverrunCount
};

enum HcInterruptEnableBits
{
	SO = 1 << 0, // SchedulingOverrun
	WDH = 1 << 1, // WritebackDoneHead
	SF = 1 << 2, // StartofFrame
	RD = 1 << 3, // ResumeDetected
	UE = 1 << 4, // UnrecoverableError
	FNO = 1 << 5, // FrameNumberOverflow
	RHSC = 1 << 6, // RootHubStatusChange
	OC = 1 << 30, // OwnershipChange
	MIE = 1 << 31, // MasterInterruptEnable
};


enum HcRhDescriptorABits
{
	NDP = 0xff << 0, // NumberDownstreamPorts
	PSM = 1 << 8, // PowerSwitchingMode
	NPS = 1 << 9, // NoPowerSwitching
	DT = 1 << 10, // DeviceType
	OCPM = 1 << 11, // OverCurrentProtectionMode
	NOCPM = 1 << 12, // NoOverCurrentProtection
	POTPGT = 0xff << 24 // PowerOnToPowerGoodTime
};

enum HcRhDescriptorBBits
{
	DR = 0xffff << 0, // DeviceRemovable
	PPCM = 0xffff << 16 // PortPowerControlMask
};

enum HcRhStatusBits
{
	LPS = 1 << 0, // LocalPowerStatus
	OCI = 1 << 1, // OverCurrentIndicator
	DRWE = 1 << 15, // DeviceRemoteWakeupEnable
	LPSC = 1 << 16, // LocalPowerStatusChange
	OCIC = 1 << 17, // OverCurrentIndicatorChange
	CRWE = 1 << 31, // ClearRemoteWakeupEnable
};

enum HcRhPortStatusBits
{
	CCS = 1 << 0, // CurrentConnectStatus
	PES = 1 << 1, // PortEnableStatus
	PSS = 1 << 2, // PortSuspendStatus
	POCI = 1 << 3, // PortOverCurrentIndicator
	PRS = 1 << 4, // PortResetStatus
	PPS = 1 << 8, // PortPowerStatus
	LSDA = 1 << 9, // LowSpeedDeviceAttached
	CSC = 1 << 16, // ConnectStatusChange
	PESC = 1 << 17, // PortEnableStatusChange
	PSSC = 1 << 18, // PortSuspendStatusChange
	POCIC = 1 << 19, // PortOverCurrentIndicatorChange
	PRSC = 1 << 20 // PortResetStatusChange
};

enum OHCIHostControllerFunctionalState {
	UsbReset=0,
	UsbResume,
	UsbOperational,
	UsbSuspend
};

enum OHCIInterrupt {
	SchedulingOverrun=1,
	WritebackDoneHead=2,
	StartofFrame=4,
	ResumeDetected=8,
	UnrecoverableError=16,
	FrameNumberOverflow=32,
	RootHubStatusChange=64,
	OwnershipChange=0x40000000,
	MasterInterruptEnable=0x80000000
};

enum OHCICompletionCode {
	NoError=0,
	CRC,
	BitStuffing,
	DataToggleMismatch,
	Stall,
	DeviceNotResponding,
	PIDCheckFailure,
	UnexpectedPID,
	DataOverrun,
	DataUnderrun,
	BufferOverrun=12,
	BufferUnderrun,
	NotAccessed=14
};

struct USBSetupPacket {
	UINT8 bmRequestType;
	UINT8 bRequest;
	UINT16 wValue;
	UINT16 wIndex;
	UINT16 wLength;
};

struct USBStandardDeviceDescriptor {
	UINT8 bLength;
	UINT8 bDescriptorType;
	UINT16 bcdUSB;
	UINT8 bDeviceClass;
	UINT8 bDeviceSubClass;
	UINT8 bDeviceProtocol;
	UINT8 bMaxPacketSize0;
	UINT16 idVendor;
	UINT16 idProduct;
	UINT16 bcdDevice;
	UINT8 iManufacturer;
	UINT8 iProduct;
	UINT8 iSerialNumber;
	UINT8 bNumConfigurations;
};

struct USBStandardConfigurationDescriptor {
	UINT8 bLength;
	UINT8 bDescriptorType;
	UINT16 wTotalLength;
	UINT8 bNumInterfaces;
	UINT8 bConfigurationValue;
	UINT8 iConfiguration;
	UINT8 bmAttributes;
	UINT8 MaxPower;
};

struct USBStandardInterfaceDescriptor {
	UINT8 bLength;
	UINT8 bDescriptorType;
	UINT8 bInterfaceNumber;
	UINT8 bAlternateSetting;
	UINT8 bNumEndpoints;
	UINT8 bInterfaceClass;
	UINT8 bInterfaceSubClass;
	UINT8 bInterfaceProtocol;
	UINT8 iInterface;
};

struct USBStandardEndpointDescriptor {
	UINT8 bLength;
	UINT8 bDescriptorType;
	UINT8 bEndpointAddress;
	UINT8 bmAttributes;
	UINT16 wMaxPacketSize;
	UINT8 bInterval;
};

enum USBPid {
	SetupPid=0,
	OutPid,
	InPid
};

enum USBRequestCode {
	GET_STATUS=0,
	CLEAR_FEATURE=1,
	SET_FEATURE=3,
	SET_ADDRESS=5,
	GET_DESCRIPTOR=6,
	SET_DESCRIPTOR=7,
	GET_CONFIGURATION=8,
	SET_CONFIGURATION=9,
	GET_INTERFACE=10,
	SET_INTERFACE=11,
	SYNCH_FRAME=12
};

enum USBDescriptorType {
	DEVICE=1,
	CONFIGURATION=2,
	STRING=3,
	INTERFACE=4,
	ENDPOINT=5
};

enum USBRequestType
{
	StandardType=0,
	ClassType,
	VendorType,
	ReservedType
};

enum USBRequestRecipient
{
	DeviceRecipient=0,
	InterfaceRecipient,
	EndpointRecipient,
	OtherRecipient
};

enum USBDeviceState
{
	DefaultState,
	AddressState,
	ConfiguredState
};

enum USBControlDirection
{
	HostToDevice=0,
	DeviceToHost=1
};

enum USBEndpointType
{
	ControlEndpoint=0,
	IsochronousEndpoint,
	BulkEndpoint,
	InterruptEndpoint
};

struct usb_device_string
{
	UINT8 *position;
	int size;
};

struct usb_device_interface_alternate
{
	UINT8 *position;
	int size;
	USBStandardInterfaceDescriptor interface_descriptor;
	std::forward_list<USBStandardEndpointDescriptor> endpoint_descriptors;
};

struct usb_device_interface
{
	UINT8 *position;
	int size;
	std::forward_list<usb_device_interface_alternate *> alternate_settings;
	int selected_alternate;
};

struct usb_device_configuration
{
	USBStandardConfigurationDescriptor configuration_descriptor;
	UINT8 *position;
	int size;
	std::forward_list<usb_device_interface *> interfaces;
};

class ohci_function_device; // forward declaration

class ohci_usb_controller : public device_t
{
public:
	ohci_usb_controller(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	~ohci_usb_controller() {}
	void usb_ohci_plug(int port, ohci_function_device *function);
	void usb_ohci_device_address_changed(int old_address, int new_address);

	template<class _Object> static devcb_base &set_interrupt_handler(device_t &device, _Object object) { return downcast<ohci_usb_controller &>(device).m_interrupt_handler.set_callback(object); }

	DECLARE_READ32_MEMBER(read);
	DECLARE_WRITE32_MEMBER(write);

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr) override;

private:
	void usb_ohci_interrupts();
	void usb_ohci_read_endpoint_descriptor(UINT32 address);
	void usb_ohci_writeback_endpoint_descriptor(UINT32 address);
	void usb_ohci_read_transfer_descriptor(UINT32 address);
	void usb_ohci_writeback_transfer_descriptor(UINT32 address);
	void usb_ohci_read_isochronous_transfer_descriptor(UINT32 address);
	void usb_ohci_writeback_isochronous_transfer_descriptor(UINT32 address);
	cpu_device *m_maincpu;
	//required_device<pic8259_device> pic8259_1;
	struct {
		UINT32 hc_regs[256];
		struct {
			ohci_function_device *function;
			int address;
			int delay;
		} ports[4 + 1];
		struct
		{
			ohci_function_device *function;
			int port;
		} address[256];
		emu_timer *timer;
		int state;
		UINT32 framenumber;
		UINT32 nextinterupted;
		UINT32 nextbulked;
		int interruptbulkratio;
		int writebackdonehadcounter;
		address_space *space;
		UINT8 buffer[1024];
		OHCIEndpointDescriptor endpoint_descriptor;
		OHCITransferDescriptor transfer_descriptor;
		OHCIIsochronousTransferDescriptor isochronous_transfer_descriptor;
	} ohcist;
	devcb_write_line m_interrupt_handler;
};

extern const device_type OHCI_USB_CONTROLLER;

#define MCFG_OHCI_USB_CONTROLLER_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, OHCI_USB_CONTROLLER, 0)
#define MCFG_OHCI_USB_CONTROLLER_INTERRUPT_HANDLER(_devcb) \
	devcb = &ohci_usb_controller::set_interrupt_handler(*device, DEVCB_##_devcb);

class ohci_function_device {
public:
	ohci_function_device();
	virtual void initialize(running_machine &machine, ohci_usb_controller *usb_bus_manager);
	virtual void execute_reset();
	virtual void execute_connect() {};
	virtual void execute_disconnect() {};
	int execute_transfer(int endpoint, int pid, UINT8 *buffer, int size);
protected:
	virtual int handle_nonstandard_request(int endpoint, USBSetupPacket *setup) { return -1; };
	virtual int handle_get_status_request(int endpoint, USBSetupPacket *setup) { return 0; };
	virtual int handle_clear_feature_request(int endpoint, USBSetupPacket *setup) { return 0; };
	virtual int handle_set_feature_request(int endpoint, USBSetupPacket *setup) { return 0; };
	virtual int handle_set_descriptor_request(int endpoint, USBSetupPacket *setup) { return 0; };
	virtual int handle_synch_frame_request(int endpoint, USBSetupPacket *setup) { return 0; };
	virtual void handle_status_stage(int endpoint) { return; };
	virtual int handle_bulk_pid(int endpoint, int pid, UINT8 *buffer, int size) { return 0; };
	virtual int handle_interrupt_pid(int endpoint, int pid, UINT8 *buffer, int size) { return 0; };
	virtual int handle_isochronous_pid(int endpoint, int pid, UINT8 *buffer, int size) { return 0; };

	void add_device_descriptor(const USBStandardDeviceDescriptor &descriptor);
	void add_configuration_descriptor(const USBStandardConfigurationDescriptor &descriptor);
	void add_interface_descriptor(const USBStandardInterfaceDescriptor &descriptor);
	void add_endpoint_descriptor(const USBStandardEndpointDescriptor &descriptor);
	void add_string_descriptor(const UINT8 *descriptor);
	void select_configuration(int index);
	void select_alternate(int interfacei, int index);
	int find_alternate(int interfacei);
	UINT8 *position_device_descriptor(int &size);
	UINT8 *position_configuration_descriptor(int index, int &size);
	UINT8 *position_string_descriptor(int index, int &size);
	ohci_usb_controller *busmanager;
	struct {
		int type;
		int controldirection;
		int controltype;
		int controlrecipient;
		int remain;
		UINT8 *position;
		UINT8 buffer[128];
	} endpoints[256];
	int state;
	bool settingaddress;
	int newaddress;
	int address;
	int configurationvalue;
	UINT8 *descriptors;
	int descriptors_pos;
	bool wantstatuscallback;
	USBStandardDeviceDescriptor device_descriptor;
	std::forward_list<usb_device_configuration *> configurations;
	std::forward_list<usb_device_string *> device_strings;
	usb_device_configuration *latest_configuration;
	usb_device_interface_alternate *latest_alternate;
	usb_device_configuration *selected_configuration;
};

extern const device_type OHCI_GAME_CONTROLLER;

class ohci_game_controller_device : public device_t, public ohci_function_device
{
public:
	ohci_game_controller_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	void initialize(running_machine &machine, ohci_usb_controller *usb_bus_manager) override;
	int handle_nonstandard_request(int endpoint, USBSetupPacket *setup) override;
	int handle_interrupt_pid(int endpoint, int pid, UINT8 *buffer, int size) override;

protected:
	virtual void device_start() override;
	virtual ioport_constructor device_input_ports() const override;
private:
	static const USBStandardDeviceDescriptor devdesc;
	static const USBStandardConfigurationDescriptor condesc;
	static const USBStandardInterfaceDescriptor intdesc;
	static const USBStandardEndpointDescriptor enddesc82;
	static const USBStandardEndpointDescriptor enddesc02;
	required_ioport m_ThumbstickLh; // left analog thumbstick horizontal movement
	required_ioport m_ThumbstickLv; // left analog thumbstick vertical movement
	required_ioport m_ThumbstickRh; // right analog thumbstick horizontal movement
	required_ioport m_ThumbstickRv; // right analog thumbstick vertical movement
	required_ioport m_DPad; // pressure sensitive directional pad
	required_ioport m_TriggerL; // analog trigger
	required_ioport m_TriggerR; // analog trigger
	required_ioport m_Buttons; // digital buttons
	required_ioport m_AGreen; // analog button
	required_ioport m_BRed; // analog button
	required_ioport m_XBlue; // analog button
	required_ioport m_YYellow; // analog button
	required_ioport m_Black; // analog button
	required_ioport m_White; // analog button
};

#include "serial/serial.h" // https://www.github.com/wjwwood/serial
#include "mak_packet.h"
#include <CombinedApi.h>
#include <iostream>
#include <unistd.h>
#xinclude <sys/ioctl.h>

using namespace std;
using namespace serial;

static CombinedApi capi = CombinedApi();
static bool apiSupportsBX2 = false;
static bool apiSupportsStreaming = false;

// function from NDI to show error codes on failureS
void onErrorPrintDebugMessage(std::string methodName, int errorCode)
{
	if (errorCode < 0)
	{
		std::cout << methodName << " failed: " << capi.errorToString(errorCode) << std::endl;
	}
}

// function from NDI to get tool info
std::string getToolInfo(std::string toolHandle)
{
	// Get the port handle info from PHINF
	PortHandleInfo info = capi.portHandleInfo(toolHandle);

	// Return the ID and SerialNumber the desired string format
	std::string outputString = info.getToolId();
	outputString.append(" s/n:").append(info.getSerialNumber());
	return outputString;
}

// function from NDI to initialize and enable tools
void initializeAndEnableTools(std::vector<ToolData>& enabledTools)
{
	std::cout << std::endl << "Initializing and enabling tools..." << std::endl;

	// Initialize and enable tools
	std::vector<PortHandleInfo> portHandles = capi.portHandleSearchRequest(PortHandleSearchRequestOption::NotInit);
	for (int i = 0; i < portHandles.size(); i++)
	{
		onErrorPrintDebugMessage("capi.portHandleInitialize()", capi.portHandleInitialize(portHandles[i].getPortHandle()));
		onErrorPrintDebugMessage("capi.portHandleEnable()", capi.portHandleEnable(portHandles[i].getPortHandle()));
	}

	// Print all enabled tools
	portHandles = capi.portHandleSearchRequest(PortHandleSearchRequestOption::Enabled);
	for (int i = 0; i < portHandles.size(); i++)
	{
		std::cout << portHandles[i].toString() << std::endl;
	}

    // Lookup and store the serial number for each enabled tool
    for (int i = 0; i < portHandles.size(); i++)
    {
        enabledTools.push_back(ToolData());
        enabledTools.back().transform.toolHandle = (uint16_t)capi.stringToInt(portHandles[i].getPortHandle());
        enabledTools.back().toolInfo = getToolInfo(portHandles[i].getPortHandle());
    }
}



int main(void){


    // ensure that we're on a linux system
    // TODO: handle Windows, darwin, etc.
    if(!__linux__){
        printf("Error: not running on a linux system\n");
        return -1;
    }

    // ensure width of float type is four bytes
    // TODO: handle other cases
    if(sizeof(float) != 4){
        printf("Error: float datatype is not four bytes wide\n");
        return -1;
    }
    //static_assert(sizeof(float)==4,"Float size is not four bytes!"); 

    // ensure little endian encoding
    // TODO: handle big endian case
    static float test_float = 1.254;
    uint8_t* p_fval = (uint8_t*) &test_float;
    if(*p_fval != 0x12){
        printf("Error: not a litle endian system\n");
        return -1;
    }
    //printf("0x%02X%02X%02X%02X\n", *p_fval, *(p_fval+1), *(p_fval+2), *(p_fval+3));


    // build a sample packet
    uint8_t mypacket[MAX_PACKET_LENGTH] = {0x00};
    size_t mypacket_length = MAX_PACKET_LENGTH;
    
    uint32_t frame_num = 4011;
    uint8_t tool_num = 2;
    float q[SIZE_Q] = {1.2,2.3,3.4,4.5};
    //float q[SIZE_Q] = {1.2,2.3,3.4,4.501953}; // force a DLE stuff in q[3]
    float t[SIZE_T] = {5.6,6.7,7.8};
    float trk_fit_err = 0.2537;
    
    int result = 0;
    result = compose_tracker_packet(mypacket, &mypacket_length, frame_num, tool_num, q, SIZE_Q, t, SIZE_T, trk_fit_err);
    if( result != 0 ){
        printf("Error: unexpected result from packet composition (%d)\n",result);
        return -1;
    }


    // display packet
    uint8_t* p_byte = mypacket;
    printf("Packet (len = %ld): ",mypacket_length); 
    while( p_byte < (mypacket + mypacket_length) ){
        printf("0x%02X ",*p_byte);
        ++p_byte;
    }
    printf("\n");


    // list serial ports 
    vector<PortInfo> all_ports = list_ports();
    if (all_ports.size() > 0) {
        cout << "Available COM ports:" << endl;
        for (unsigned int i = 0; i < all_ports.size(); i++) {
            cout << "* " << all_ports[i].port << " (" << all_ports[i].description << " / " << all_ports[i].hardware_id << ")" << endl;
        }
        cout << endl;
    }

    // open serial port for connection to client device (not the tracker)
    cout << "Attempting to open /dev/ttyUSB0...";
    Serial* mySerialPort = NULL;
    try {
        mySerialPort = new Serial("/dev/ttyUSB0", 115200U, Timeout(50,200,3,200,3), eightbits, parity_none, stopbits_one, flowcontrol_none);
    } catch(IOException const& e){
        cout << e.what() << endl;
        return -1;
    };
    mySerialPort->flush();
    cout << "done." << endl;

    
    // open connection to trakcer
    // TODO: fix port, ideally figure it out dynamically
    if(capi.connect("/dev/ttyUSB0") != 0){
        printf("Error connecting to NDI API...\r");
    }

    // sleep for a second (NDI required?)
    sleep(1);    

    // get NDI device firmware version?
    cout << capi.getUserParameter("Features.Firmware.Version") << endl;

    // attempt to initialize NDI common API
    if( (result = capi.initialize()) < 0 ){
        cout << "Error initializing NDI common api: " << capi.errorToString(result) << endl;
        return -1;
    }

    // initialize and enable tools
    std::vector<ToolData> enabledTools = std::vector<ToolData>();
    initializeAndEnableTools(enabledTools);

    
    // write packet to serial port
    mySerialPort->write(mypacket,mypacket_length);


    // close serial port
    cout << "Closing serial port" << endl;
    mySerialPort->close();

    // done	
    return 0;
}

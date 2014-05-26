// Expire time for registrations
expires = 3600;

// ip:port where SIP requests are sent
// reg_sip = "192.168.1.245:5058"; 
// It is REQUIRED to set reg_sip or nodes_sip
reg_sip = "";


// openvolte servers
// node => ip:port of each server
// node is computed based on the tmsi received from the handset. 
// This ensures that registrations are always sent to the same openvolte server
// It is REQUIRED to set reg_sip or nodes_sip
/* Example:
nodes_sip = {
         "123":"192.168.1.245:5058",
         "170":"192.168.1.176:5059"
};
nnsf_bits = 8;
*/

// ip:port for local SIP listener
// Unless otherwise configured this is the ip of the machine where yatebts is installed
// my_sip = "192.168.1.168";
// REQUIRED!
my_sip = "";

// unique number associated to each base station by the network operator
// gstn_location = "40740123456";
// REQUIRED!
gstn_location = "";

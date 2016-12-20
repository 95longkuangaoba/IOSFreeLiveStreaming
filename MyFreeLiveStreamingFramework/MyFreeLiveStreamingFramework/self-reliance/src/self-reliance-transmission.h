#ifndef ___SELF_RELIANCE_TRANSMISSION_H___
#define ___SELF_RELIANCE_TRANSMISSION_H___



enum
{
	SR___Transmission_Type___Request = 0,
	SR___Transmission_Type___Respond = 1
};


enum
{
	SR___Transmission_Unit_Type___Raw = 0,
	SR___Transmission_Unit_Type___Ping,
	SR___Transmission_Unit_Type___Resend,
	SR___Transmission_Unit_Type___Termination = 0x7F
};


struct sr___transmission_unit___t
{
	uint32_t checksum;
	uint8_t transmission_type;
	uint8_t unit_type;
	uint64_t serial_number;
};



#endif //___SELF_RELIANCE_TRANSMISSION_H___

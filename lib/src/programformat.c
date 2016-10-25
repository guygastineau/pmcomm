#include "programformat.h"

#include <string.h>

enum PMProgramFormat PMProgramDataFormat(struct PMConnection *conn, int addr) {
	switch(addr) {
		case 0xff:
		case 0xfe:
		case 0xfd:
		case 0xfc:
		case 0xfb: return PM_PROGFORMAT_SWITCHDISP;
		case 0xe1: return PM_PROGFORMAT_LABELS;
		case 0xf6: return PM_PROGFORMAT_SHUNTTYPE;
		case 0xf7: return PM_PROGFORMAT_VERSION;
		case 0xf2:
		case 0xf1: return PM_PROGFORMAT_CAPACITY;
		case 0xf3: return PM_PROGFORMAT_FILTERTIME;
		case 0xce:
		case 0xcd: return PM_PROGFORMAT_ALARMLEVEL;
		case 0xec: return PM_PROGFORMAT_LOSETPOINT;
		case 0xea: return PM_PROGFORMAT_HISETPOINT;
		case 0xeb: return PM_PROGFORMAT_LOSETPOINT;
		case 0xe9: return PM_PROGFORMAT_HISETPOINT;
		case 0xd4: return PM_PROGFORMAT_RELAYSETPOINT;
		case 0xe8:
		case 0xe7: return PM_PROGFORMAT_CHARGEDCRITERIA;
		case 0xe5: return PM_PROGFORMAT_EFFICIENCY;
		case 0xe3:
		case 0xe2: return PM_PROGFORMAT_CHARGETIME;
		case 0xf9: return PM_PROGFORMAT_TIMEMINUTES;
		case 0xcf: return PM_PROGFORMAT_STARTTIME;
		case 0xd0: return PM_PROGFORMAT_MEASPERDAY;
		case 0xd2: return PM_PROGFORMAT_LOGGEDITEMS;
		case 0xd1: return PM_PROGFORMAT_FIVEPERCENT;

		case 0x90: return PM_PROGFORMAT_TCP;
		case 0x91: return PM_PROGFORMAT_TCP2;
		case 0x92: return PM_PROGFORMAT_TCPSTRING;
		case 0x93: return PM_PROGFORMAT_TCPSTRING;
		
		default: return PM_PROGFORMAT_INVALID;
	}
}

int PMFormatProgramData(unsigned char *data, unsigned char *data2, union PMProgramData *result, enum PMProgramFormat format) {
	int i;
	switch(format) {
		case PM_PROGFORMAT_SWITCHDISP:
			for(i = 0; i < 5; i++) {
				result->switchDisplay[i] = (enum PMDisplayNumber) data[i];
			}
			return 0;
		case PM_PROGFORMAT_LABELS:
			result->labels.battLabel = (data[1] & 0x10) ? PM_BATLABEL_BATTERY : PM_BATLABEL_BATTERY_1;
			for(i = 0; i < 3; i++) {
				result->labels.ampsLabels[i] = data[i] & 0x7;
			}
			return 0;
		case PM_PROGFORMAT_SHUNTTYPE:
			for(i = 0; i < 3; i++) {
				result->shunts.types[i] = (data[i] & 0x10) ? PM_SHUNTTYPE_500A50MV : PM_SHUNTTYPE_100A100MV;
			}
			result->shunts.amps2Sum = (data[1] & 0x20);
			return 0;
		case PM_PROGFORMAT_VERSION:
			result->version = data[0];
			return 0;
		case PM_PROGFORMAT_CAPACITY:
			result->batCapacity = data[0] | (data[1] << 8);
			return 0;
		case PM_PROGFORMAT_FILTERTIME:
			result->filterTime = data[0] & 0x7;
			return 0;
		case PM_PROGFORMAT_ALARMLEVEL:
			for(i = 0; i < 4; i++) {
				result->alarmLevel[i] = (data[0] >> (2 * i)) & 0x3;
			}
			result->alarmLevel[4] = data[1] & 0x3;
			return 0;
		case PM_PROGFORMAT_LOSETPOINT:
			result->lowAlarmSetpoint.voltage = data[0] | ((data[1] & 0x3) << 8);
			result->lowAlarmSetpoint.percent = data[2];
			return 0;
		case PM_PROGFORMAT_HISETPOINT:
			result->highAlarmSetpoint = data[0] | ((data[1] & 0x3) << 8);
			return 0;
		case PM_PROGFORMAT_RELAYSETPOINT:
			result->relaySetpoint[0].voltage = data[0] | ((data[1] & 0x3) << 8);
			result->relaySetpoint[0].percent = data[2];
			result->relaySetpoint[1].voltage = data[3] | ((data[4] & 0x3) << 8);
			result->relaySetpoint[1].percent = data[5];
			return 0;
		case PM_PROGFORMAT_CHARGEDCRITERIA:
			result->chargedCriteria.voltage = data[0] | ((data[1] & 0x3) << 8);
			result->chargedCriteria.current = data[2];
			return 0;
		case PM_PROGFORMAT_EFFICIENCY:
			result->efficiency.percent = data[0];
			result->efficiency.selfdischarge = data[1] | (data[2] << 8);
			return 0;
		case PM_PROGFORMAT_CHARGETIME:
			result->chargeTime = data[0];
			return 0;
		case PM_PROGFORMAT_TIMEMINUTES:
			result->timeMinutes = (data[0] | (data[1] << 8)) * 180 + data2[0];
			return 0;
		case PM_PROGFORMAT_STARTTIME:
			result->timeMinutes = data[0] * 180 + data[2];
			return 0;
		case PM_PROGFORMAT_MEASPERDAY:
			result->minutesPerMeasurement = ((~data[0] & 0x1) + 1) * ((~(data[0] >> 1) & 1) + 1) * 
											((~(data[0] >> 2) & 1) + 1) * ((~(data[0] >> 3) & 1) + 1) * 
											((~(data[0] >> 4) & 1) + 1) * ((~(data[0] >> 4) & 2) + 1) * 
											((~(data[0] >> 5) & 2) + 1) * ((~(data[0] >> 5) & 4) + 1);
			return 0;
		case PM_PROGFORMAT_LOGGEDITEMS:
			result->periodicLoggedItems = data[0] | ((data[1] & 0x3) << 8);
			return 0;
		case PM_PROGFORMAT_FIVEPERCENT:
			result->fivePercentOptions = data[0] & 0xe0;
			return 0;

		case PM_PROGFORMAT_TCP:
			result->tcp.dhcpEnabled = data[0] & 0x1;
			memcpy(result->tcp.ipAddress, data + 1, 4);
			memcpy(result->tcp.gatewayAddress, data + 5, 4);
			for(i = 0; i < 4; i++) {
				int shift = data[9] - 8 * i;
				if(shift > 8)
					shift = 8;
				if(shift < 0)
					shift = 0;
				result->tcp.subnetMask[3 - i] = 0xFF << shift;
			}
			memcpy(result->tcp.primaryDNSServer, data + 10, 4);
			return 0;
		case PM_PROGFORMAT_TCP2:
			memcpy(result->tcp2.secondaryDNSServer, data, 4);
			result->tcp2.listenPort = data[4] | (data[5] << 8);
			return 0;
		case PM_PROGFORMAT_TCPSTRING:
			result->tcpString[16] = 0;
			strncpy(result->tcpString, (char *) data, 16);
			return 0;

		default:
			return -1;
	}
}

int PMEncodeProgramData(unsigned char *data, unsigned char *data2, union PMProgramData *input, enum PMProgramFormat format) {
	int i, minutes;
	switch(format) {
		case PM_PROGFORMAT_SWITCHDISP:
			for(i = 0; i < 5; i++) {
				if(input->switchDisplay[i] > PM_D40)
					return -1;
				data[i] = input->switchDisplay[i];
			}
			return 0;
		case PM_PROGFORMAT_LABELS:
			for(i = 0; i < 3; i++) {
				if(input->labels.ampsLabels[i] > PM_AMPSLABEL_BATTERY_1)
					return -1;
				data[i] = input->labels.ampsLabels[i];
			}
			if(input->labels.battLabel == PM_BATLABEL_BATTERY)
				data[1] |= 0x10;
			return 0;
		case PM_PROGFORMAT_SHUNTTYPE:
			for(i = 0; i < 3; i++) {
				data[i] = (input->shunts.types[i] == PM_SHUNTTYPE_500A50MV) ? 0x10 : 0;
			}
			if(input->shunts.amps2Sum)
				data[1] |= 0x20;
			return 0;
		case PM_PROGFORMAT_VERSION:
			return -1;
		case PM_PROGFORMAT_CAPACITY:
			if(input->batCapacity > 9999)
				return -1;
			data[0] = input->batCapacity % 256;
			data[1] = input->batCapacity / 256;
			return 0;
		case PM_PROGFORMAT_FILTERTIME:
			if(input->filterTime > PM_FILTERTIME_32MIN || input->filterTime < PM_FILTERTIME_30SEC)
				return -1;
			data[0] = input->filterTime;
			return 0;
		case PM_PROGFORMAT_ALARMLEVEL:
			data[0] = 0;
			for(i = 0; i < 4; i++) {
				if(input->alarmLevel[i] > PM_ALARMLEVEL_VISUAL_AUDIBLE || input->alarmLevel[i] < PM_ALARMLEVEL_OFF)
					return -1;
				data[0] |= (input->alarmLevel[i] & 0x3) << (2 * i);
			}
			if(input->alarmLevel[i] > PM_ALARMLEVEL_VISUAL_AUDIBLE || input->alarmLevel[i] < PM_ALARMLEVEL_OFF)
				return -1;
			data[1] = input->alarmLevel[4] & 0x3;
			return 0;
		case PM_PROGFORMAT_LOSETPOINT:
			data[0] = input->lowAlarmSetpoint.voltage % 256;
			if(input->lowAlarmSetpoint.voltage / 256 > 3)
				return -1;
			data[1] = (input->lowAlarmSetpoint.voltage / 256) & 0x3;
			if(input->lowAlarmSetpoint.percent > 100)
				return -1;
			data[2] = input->lowAlarmSetpoint.percent;
			return 0;
		case PM_PROGFORMAT_HISETPOINT:
			data[0] = input->lowAlarmSetpoint.voltage % 256;
			if(input->lowAlarmSetpoint.voltage / 256 > 3)
				return -1;
			data[1] = (input->lowAlarmSetpoint.voltage / 256) & 0x3;
			return 0;
		case PM_PROGFORMAT_RELAYSETPOINT:
			for(i = 0; i < 2; i++) {
				data[0 + 3 * i] = input->relaySetpoint[i].voltage % 256;
				if(input->relaySetpoint[i].voltage / 256 > 3)
					return -1;
				data[1 + 3 * i] = (input->relaySetpoint[i].voltage / 256) & 0x3;
				if(input->relaySetpoint[i].percent > 100)
					return -1;
				data[2 + 3 * i] = input->relaySetpoint[i].percent;
			}
			return 0;
		case PM_PROGFORMAT_CHARGEDCRITERIA:
			data[0] = input->chargedCriteria.voltage % 256;
			if(input->chargedCriteria.voltage / 256 > 3)
				return -1;
			data[1] = input->chargedCriteria.voltage / 256;
			if(input->chargedCriteria.current > 99) // Check
				return -1;
			data[2] = input->chargedCriteria.current;
			return 0;
		case PM_PROGFORMAT_EFFICIENCY:
			if(input->efficiency.percent > 100 || input->efficiency.percent < 60)
				return -1;
			data[0] = input->efficiency.percent;
			data[1] = input->efficiency.selfdischarge % 256;
			if(input->efficiency.selfdischarge > 999)
				return -1;
			data[2] = input->efficiency.selfdischarge / 256;
			return 0;
		case PM_PROGFORMAT_CHARGETIME:
			data[0] = input->chargeTime;
			return 0;
		case PM_PROGFORMAT_TIMEMINUTES:
			data[0] = (input->timeMinutes / 180) % 256;
			if(input->timeMinutes / 180 > 65535)
				return -1;
			data[1] = (input->timeMinutes / 180) / 256;
			data2[0] = input->timeMinutes % 180;
			return 0;
		case PM_PROGFORMAT_STARTTIME:
			if(input->timeMinutes > 1440)
				return -1;
			data[0] = input->timeMinutes / 180;
			data[1] = 0;
			data[2] = input->timeMinutes % 180;
			return 0;
		case PM_PROGFORMAT_MEASPERDAY:
			minutes = input->minutesPerMeasurement;
			data[0] = 0xff;
			for(i = 0; i < 5; i++) {
				if(minutes % 2 == 0) {
					data[0] &= ~(1 << i);
					minutes /= 2;
				}
			}
			for(i = 0; i < 2; i++) {
				if(minutes % 3 == 0) {
					data[0] &= ~(1 << (i + 5));
					minutes /= 3;
				}
			}
			if(minutes % 5 == 0) {
				data[0] &= ~(1 << 7);
				minutes /= 5;
			}
			if(minutes != 1)
				return -1;
			return 0;
		case PM_PROGFORMAT_LOGGEDITEMS:
			data[0] = input->periodicLoggedItems % 256;
			if(input->periodicLoggedItems >= 0x400)
				return -1;
			data[1] = input->periodicLoggedItems / 256;
			return 0;
		case PM_PROGFORMAT_FIVEPERCENT:
			data[0] = input->fivePercentOptions & 0xe0;
			data[0] |= data2[0] & 0x1f;
			return 0;

		case PM_PROGFORMAT_TCP:
			data[0] = input->tcp.dhcpEnabled ? 1 : 0;
			memcpy(data + 1, input->tcp.ipAddress, 4);
			memcpy(data + 5, input->tcp.gatewayAddress, 4);
			data[9] = 32;
			{
				bool foundOne = false; // First one already found
				for(i = 0; i < 4; i++) {
					int j;
					for(j = 0; j < 8; j++) {
						bool isOne = (input->tcp.subnetMask[3 - i] & (1 << j)) != 0;
						if(!foundOne && isOne) {
							data[9] = i * 8 + j;
							foundOne = true;
						} else if(foundOne && !isOne) { // Went back to 0
							return -1;
						}
					}
				}
			}
			memcpy(data + 10, input->tcp.primaryDNSServer, 4);
			return 0;
		case PM_PROGFORMAT_TCP2:
			memcpy(data, input->tcp2.secondaryDNSServer, 4);
			data[4] = input->tcp2.listenPort % 256;
			data[5] = input->tcp2.listenPort / 256;
			return 0;
		case PM_PROGFORMAT_TCPSTRING:
			strncpy((char *) data, input->tcpString, 16); // This may not be nul-terminated, which is deliberate
			return 0;

		default:
			return -1;
	}
}
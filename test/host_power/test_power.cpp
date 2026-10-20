#include "p1_reader.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <cstdio>
static int checks=0;
void check(bool ok,const char* msg){++checks;if(!ok)throw std::runtime_error(msg);}
void near(float actual,float expected,const char* msg){check(std::fabs(actual-expected)<0.001f,msg);}
std::string frame(const char* fields) {
 std::string s=std::string("/TEST\r\n0-0:1.0.0(260930120000S)\r\n")+fields+"!";
 unsigned crc=0;
 for(unsigned char c:s){crc^=c;for(int n=0;n<8;n++)crc=(crc>>1)^((crc&1)?0xA001:0);}
 char hex[5];std::snprintf(hex,sizeof(hex),"%04X",crc);
 return s+hex+"\r\n";
}
P1Data read(const char* fields){return P1Reader::parseTelegram(String(frame(fields).c_str()));}
int main(){
 try {
 P1Data latest;
 auto a=read("1-0:1.7.0(12.000*kW)\r\n1-0:2.7.0(00.000*kW)\r\n");
 check(a.valid && a.hasPowerConsumed && a.hasPowerProduced,"valid totals presence");
 latest.mergeActivePowerFrom(a);
 near(latest.powerConsumed,12,"initial 12kW");
 auto b=read("1-0:1.7.0(00.000*kW)\r\n1-0:2.7.0(01.359*kW)\r\n");
 latest.mergeActivePowerFrom(b);
 near((latest.powerConsumed-latest.powerProduced)*1000,-1359,"10641W regression");
 auto c=read("1-0:1.7.0(01.790*kW)\r\n1-0:2.7.0(00.000*kW)\r\n");
 latest.mergeActivePowerFrom(c);
 near((latest.powerConsumed-latest.powerProduced)*1000,1790,"export returns to zero");
 latest.mergeActivePowerFrom(b);
 auto missing=read("");
 check(missing.valid && !missing.hasPowerConsumed && !missing.hasPowerProduced,"absent fields marked absent");
 latest.mergeActivePowerFrom(missing);
 near(latest.powerProduced,1.359f,"missing field preserves value");
 auto partial=read("1-0:1.7.0(02.000*kW)\r\n");
 latest.mergeActivePowerFrom(partial);
 near(latest.powerConsumed,2,"partial import updates");
 near(latest.powerProduced,1.359f,"partial export preserved");
 const char* bad[]={"", "abc", "1.2junk", "-1.0", "nan", "inf", "1e2", "1.2.3", "1.", "999999999999999999999999999999999999999999999999999"};
 for(auto value:bad){
 auto d=read((std::string("1-0:1.7.0(")+value+"*kW)\r\n").c_str());
 check(!d.hasPowerConsumed,"malformed import flagged");latest.mergeActivePowerFrom(d);near(latest.powerConsumed,2,"malformed import preserved");
 auto e=read((std::string("1-0:2.7.0(")+value+"*kW)\r\n").c_str());
 check(!e.hasPowerProduced,"malformed export flagged");latest.mergeActivePowerFrom(e);near(latest.powerProduced,1.359f,"malformed export preserved");
 }
 for(auto field:{"1-0:1.7.0(1.0*W)\r\n","1-0:1.7.0(1.0*kW\r\n","1-0:1.7.0(1.0*kW)junk\r\n"}) check(!read(field).hasPowerConsumed,"wrong unit/framing rejected");
 auto zero=read("1-0:1.7.0(0*kW)\r\n1-0:2.7.0(0.000*kW)\r\n");
 latest.mergeActivePowerFrom(zero);near(latest.powerConsumed,0,"import zero accepted");near(latest.powerProduced,0,"export zero accepted");

 auto full=read("1-0:32.7.0(194.0*V)\r\n1-0:52.7.0(129.0*V)\r\n1-0:72.7.0(108.0*V)\r\n");
 check(full.valid && full.hasVoltageL1 && full.hasVoltageL2 && full.hasVoltageL3,"three voltages available");
 latest.updateVoltagesFrom(full);near(latest.voltageL1,194,"voltage value");
 auto partialVoltage=read("1-0:52.7.0(129.0*V)\r\n1-0:72.7.0(108.0*V)\r\n");
 latest.updateVoltagesFrom(partialVoltage);
 check(!latest.hasVoltageL1 && latest.hasVoltageL2 && latest.hasVoltageL3,"missing L1 unavailable");
 latest.updateVoltagesFrom(read(""));
 check(!latest.hasVoltageL1 && !latest.hasVoltageL2 && !latest.hasVoltageL3,"missing all unavailable");
 for(auto field:{"abc*V)","-1*V)","230x*V)","230*W)","230*V", "nan*V)","230*V)junk"}) {
 auto v=read((std::string("1-0:32.7.0(")+field+"\r\n").c_str());
 check(v.valid && !v.hasVoltageL1,"malformed voltage unavailable");
 }
 auto realZero=read("1-0:32.7.0(0.0*V)\r\n");
 check(realZero.hasVoltageL1,"real zero available");near(realZero.voltageL1,0,"real zero retained");
 latest.updateVoltagesFrom(full);check(latest.hasVoltageL1,"voltage recovers");
 std::string good=frame("1-0:32.7.0(194.0*V)\r\n");
 check(P1Reader::parseTelegram(String(good.c_str())).valid,"valid CRC accepted");
 std::string corrupt=good;corrupt[corrupt.find("194.0")]='2';
 check(!P1Reader::parseTelegram(String(corrupt.c_str())).valid,"payload corruption rejected");
 for(auto suffix:{"", "123", "ZZZZ", "0000"}) {
 auto broken=good.substr(0,good.find('!')+1)+suffix;
 check(!P1Reader::parseTelegram(String(broken.c_str())).valid,"bad CRC rejected");
 }
 check(P1Reader::parseTelegram(String("/ELS5\r\n\r\n0-0:96.1.4(50221)\r\n1-0:94.32.1(230)\r\n0-0:96.1.1(534D363238383830303031)\r\n0-0:96.1.2(37343332303234313837313136)\r\n0-0:1.0.0(260930130855S)\r\n1-0:1.8.1(000004.480*kWh)\r\n1-0:1.8.2(000000.307*kWh)\r\n1-0:2.8.1(000002.165*kWh)\r\n1-0:2.8.2(000017.642*kWh)\r\n1-0:1.4.0(00.000*kW)\r\n1-0:1.6.0(250408103912S)(00.000*kW)\r\n0-0:98.1.0(1)(1-0:1.6.0)(1-0:1.6.0)(250501000000S)(250408034825S)(00.000*kW)\r\n0-0:96.14.0(0001)\r\n1-0:1.7.0(00.000*kW)\r\n1-0:2.7.0(01.359*kW)\r\n1-0:21.7.0(00.000*kW)\r\n1-0:41.7.0(00.000*kW)\r\n1-0:61.7.0(00.000*kW)\r\n1-0:22.7.0(00.453*kW)\r\n1-0:42.7.0(00.453*kW)\r\n1-0:62.7.0(00.453*kW)\r\n0-0:96.3.10(1)\r\n0-0:17.0.0(99.999*kW)\r\n1-0:31.4.0(999.99*A)\r\n0-1:96.3.10(0)\r\n0-2:96.3.10(1)\r\n0-3:96.3.10(0)\r\n0-4:96.3.10(0)\r\n0-0:96.13.0(4C6F72656D20697073756D202D2050312074657874206D657373616765)\r\n1-0:32.7.0(194.0*V)\r\n1-0:52.7.0(129.0*V)\r\n1-0:72.7.0(108.0*V)\r\n1-0:31.7.0(002.34*A)\r\n1-0:51.7.0(003.51*A)\r\n1-0:71.7.0(004.19*A)\r\n0-1:24.1.0(003)\r\n0-1:96.1.1(4D2D4944322D30303141)\r\n0-1:96.1.2(37343332303234313030303031)\r\n0-1:24.4.0(1)\r\n0-1:24.2.3(260930130829S)(00000.950*m3)\r\n!F9B7")).valid,"captured simulator CRC accepted");
 std::cout<<checks<<" checks passed\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}

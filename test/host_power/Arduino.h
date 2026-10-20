#pragma once
#include <string>
#include <cstdint>
#include <cstdlib>
#include <algorithm>
class String {
 std::string s;
public:
 String() = default;
 String(const char* v):s(v) {}
 String(const std::string& v):s(v) {}
 template<class T> String(T v):s(std::to_string(v)) {}
 const char* c_str() const { return s.c_str(); }
 int length() const { return static_cast<int>(s.size()); }
 char operator[](int i) const { return s[i]; }
 char charAt(int i) const { return s[i]; }
 int indexOf(char c,int pos=0) const { auto n=s.find(c,pos);return n==s.npos?-1:static_cast<int>(n); }
 int indexOf(const char* c,int pos=0) const { auto n=s.find(c,pos);return n==s.npos?-1:static_cast<int>(n); }
 String substring(int from,int to) const { return s.substr(from,to-from); }
 String substring(int from) const { return s.substr(from); }
 float toFloat() const { return std::strtof(s.c_str(),nullptr); }
 long toInt() const { return std::strtol(s.c_str(),nullptr,10); }
 void trim() { auto a=s.find_first_not_of(" \r\n\t"); if(a==s.npos){s.clear();return;} auto b=s.find_last_not_of(" \r\n\t");s=s.substr(a,b-a+1); }
 void replace(const char* a,const char* b) { size_t pos=0;std::string x(a),y(b);while((pos=s.find(x,pos))!=s.npos){s.replace(pos,x.size(),y);pos+=y.size();} }
 friend String operator+(const String& a,const String& b){return a.s+b.s;}
};

#pragma once
#include <winhttp.h>
#include <future>
#include <chrono>
#pragma comment(lib,"Winhttp.lib")
struct MarketObservation {std::string symbol;double price=0;std::string asOfDate,source;};
struct InternetHandle {HINTERNET handle=nullptr;explicit InternetHandle(HINTERNET h):handle(h){if(!handle)throw std::runtime_error("Could not initialize the market-data connection.");}~InternetHandle(){if(handle)WinHttpCloseHandle(handle);}InternetHandle(const InternetHandle&)=delete;};
static std::string marketQueryEncode(const std::string& s){std::string out;const char* hex="0123456789ABCDEF";for(unsigned char c:s){if(std::isalnum(c)||c=='-'||c=='_'||c=='.')out+=c;else{out+='%';out+=hex[c>>4];out+=hex[c&15];}}return out;}
static MarketObservation fetchMarketObservation(const std::string& symbol,const std::string& key) {
    InternetHandle session(WinHttpOpen(L"Aurelia-CS499/1.0",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0));WinHttpSetTimeouts(session.handle,5000,5000,5000,10000);
    InternetHandle connection(WinHttpConnect(session.handle,L"www.alphavantage.co",INTERNET_DEFAULT_HTTPS_PORT,0));
    std::string path="/query?function=GLOBAL_QUOTE&symbol="+marketQueryEncode(symbol)+"&apikey="+marketQueryEncode(key)+"&datatype=csv";std::wstring wide(path.begin(),path.end());
    InternetHandle request(WinHttpOpenRequest(connection.handle,L"GET",wide.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE));
    if(!WinHttpSendRequest(request.handle,WINHTTP_NO_ADDITIONAL_HEADERS,0,WINHTTP_NO_REQUEST_DATA,0,0,0)||!WinHttpReceiveResponse(request.handle,nullptr))throw std::runtime_error("Market provider is unavailable. Stored observations were retained.");DWORD status=0,len=sizeof(status);WinHttpQueryHeaders(request.handle,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&len,WINHTTP_NO_HEADER_INDEX);if(status!=200)throw std::runtime_error("Market provider returned a non-success status.");
    std::string body;for(;;){DWORD available=0;if(!WinHttpQueryDataAvailable(request.handle,&available))throw std::runtime_error("Market response failed.");if(!available)break;if(body.size()+available>20000)throw std::runtime_error("Unexpected market response size.");std::vector<char> buffer(available);DWORD read=0;if(!WinHttpReadData(request.handle,buffer.data(),available,&read))throw std::runtime_error("Market response was interrupted.");body.append(buffer.data(),read);}
    std::istringstream input(body);std::string header,row;std::getline(input,header);std::getline(input,row);if(header.find("latestDay")==std::string::npos && header.find("latest trading day")==std::string::npos)throw std::runtime_error("No quote returned. Check the local API key, subscription and provider rate limit.");
    std::vector<std::string> cells;std::istringstream line(row);std::string cell;while(std::getline(line,cell,','))cells.push_back(cell);if(cells.size()<7 || cells[0]!=symbol)throw std::runtime_error("The provider response did not match the requested security.");MarketObservation o{symbol,std::stod(cells[4]),cells[6],"https://www.alphavantage.co/documentation/#latestprice"};if(!o.asOfDate.empty()&&o.asOfDate.back()=='\r')o.asOfDate.pop_back();return o;
}


#include "ESClient.h"
int ESClient::postRequest(ESClientRequestMethod method, const std::string &url, const std::string &body, std::string &response) 
{
    auto request = std::make_shared<TC_HttpRequest>();
    request->setHost(_host);
    switch (method) 
    {
        case ESClientRequestMethod::Post: 
        {
            request->setHeader("Content-Type", "application/x-ndjson");

            request->setPostRequest(url, body);
        }
        break;
        case ESClientRequestMethod::Put: 
        {
            request->setHeader("Content-Type", "application/json");
            request->setPutRequest(url, body);
        }
        break;
        default:
        case ESClientRequestMethod::Get: 
        {
            request->setHeader("Content-Type", "application/json");
            //tc_http GET Method Not Support Body ,So We Use POST Instead.
            request->setPostRequest(url, body);
        }
        break;
    }

    auto pResponse = std::make_shared<TC_HttpResponse>();
    try 
    {
        _esPrx->http_call("es", request, pResponse);
    }
    catch (const std::exception &e) 
    {
        response = e.what();
        return -1;
    }
    response = pResponse->getContent();
    return pResponse->getStatus();
}

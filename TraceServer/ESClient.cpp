#include <cstring>
// #include <asio/posix/stream_descriptor.hpp>
#include <utility>
#include "ESClient.h"

// constexpr size_t MAX_ES_CLIENT_SESSION_SIZE = 5;
// static std::array<uint8_t, 8> NotifyBuffer{"2323232"};

// std::string ESClient::ESHost;
// int ESClient::ESPort = 0;

// class ESClientRequestEntry {
// public:
//     void initHttpResponseParser() {
//         memset(&httpResponseParser_, 0, sizeof(httpResponseParser_));
//         http_parser_init(&httpResponseParser_, HTTP_RESPONSE);
//         httpResponseParser_.data = this;
//     }

//     std::string httpRequestContent_{};
//     std::string httpResponseBody_{};
//     http_parser httpResponseParser_{};
//     bool httpResponseComplete_{false};
// };

// const char *ESClientRequest::responseBody() const {
//     return entry_->httpResponseBody_.data();
// }

// size_t ESClientRequest::responseSize() const {
//     return entry_->httpResponseBody_.size();
// }

// unsigned int ESClientRequest::responseCode() const {
//     return entry_->httpResponseParser_.status_code;
// }

// class ESClientWorker {
// public:
//     ESClientWorker(std::string host, int port, asio::io_context &ioContext, std::queue<std::shared_ptr<ESClientRequest>> &pendingQueue,
//                    asio::posix::stream_descriptor &eventStream) :
//             host_(std::move(host)),
//             port_(port),
//             ioContext_(ioContext),
//             pendingTaskQueue_(pendingQueue),
//             eventStream_(eventStream) {

//         memset(&responseParserSetting_, 0, sizeof(responseParserSetting_));

//         responseParserSetting_.on_body = ([](http_parser *p, const char *d, size_t l) -> int {
//             auto *taskEntry = static_cast<ESClientRequestEntry *const>(p->data);
//             taskEntry->httpResponseBody_.append(d, l);
//             return 0;
//         });

//         responseParserSetting_.on_message_complete = [](http_parser *p) -> int {
//             auto *taskEntry = static_cast<ESClientRequestEntry *const>(p->data);
//             taskEntry->httpResponseComplete_ = true;
//             return 0;
//         };
//     };

//     void run() {
//         doWork();
//     }

// private:
//     void doWork() {
//         if (pendingTaskQueue_.empty()) {
//             return doWaitNextTask();
//         }

//         runningTask_ = pendingTaskQueue_.front();
//         pendingTaskQueue_.pop();

//         if (!pendingTaskQueue_.empty()) {
//             eventStream_.async_write_some(notifyBufferWrapper_,
//                                           [](const std::error_code &ec, std::size_t) {
//                                               if (ec) {
//                                                   throw std::runtime_error(std::string(
//                                                           "fatal error occurred while writing eventStream :").append(
//                                                           ec.message()));
//                                               }
//                                           });
//         }

//         if (stream_ == nullptr) {
//             stream_.reset(new asio::ip::tcp::socket(ioContext_));
//             doConnect();
//             return;
//         }

//         if (!stream_->is_open()) {
//             doConnect();
//             return;
//         }

//         doWriteRequest();
//     }

//     void doConnect() {
//         if (host_.empty()) {
//             throw std::runtime_error("fatal error: empty es server host value");
//         }
//         if (port_ <= 0 || port_ > 65535) {
//             throw std::runtime_error(std::string("fatal error: bad es server port value|").append(std::to_string(port_)));
//         }
//         asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string(host_), port_);
//         stream_->async_connect(endpoint, [this](asio::error_code ec) { afterConnect(ec); });
//     }

//     void afterConnect(const asio::error_code &ec) {
//         if (ec) {
//             std::cout << "do connect fail" << std::endl;
//             return afterSocketFail(std::string("connect to server error: ").append(ec.message()));
//         }
//         doWriteRequest();
//     }


//     void doWriteRequest() {
//         auto task = runningTask_.lock();
//         if (task == nullptr) { //task may be canceled;
//             doWaitNextTask();
//             return;
//         }
//         task->setPhase(ESClientRequestPhase::Running, "Writing RequestContent To Server");
//         auto taskEntry = task->entry_;
//         asio::async_write(*stream_, asio::buffer(taskEntry->httpRequestContent_),
//                           [taskEntry, this](asio::error_code ec, std::size_t bytes_transferred) {
//                               afterWriteRequest(ec);
//                           });
//     }

//     void afterWriteRequest(const asio::error_code &ec) {
//         if (ec) {
//             return afterSocketFail(std::string("write to server error: ").append(ec.message()));
//         }
//         responseBuffer_.consume(responseBuffer_.size());
//         doReadResponse();
//     }

//     void doReadResponse() {
//         auto task = runningTask_.lock();
//         if (task == nullptr) {  //task may be canceled;
//             doWaitNextTask();
//             return;
//         }

//         task->setPhase(ESClientRequestPhase::Running, "Reading ResponseContent From Server");
//         stream_->async_read_some(responseBuffer_.prepare(1024 * 1024 * 4),
//                                  [this](asio::error_code ec, size_t bytes_transferred) {
//                                      afterReadResponse(ec, bytes_transferred);
//                                  });
//     }

//     void afterReadResponse(const asio::error_code &ec, std::size_t bytes_transferred) {
//         if (ec) {
//             return afterSocketFail(std::string("read from server error: ").append(ec.message()));
//         }
//         responseBuffer_.commit(bytes_transferred);

//         auto task = runningTask_.lock();
//         if (task == nullptr) {  //task may be canceled;
//             doWaitNextTask();
//             return;
//         }

//         const char *willParseData = asio::buffer_cast<const char *>(responseBuffer_.data());
//         size_t parserLength = http_parser_execute(&task->entry_->httpResponseParser_, &responseParserSetting_,
//                                                   willParseData, responseBuffer_.size());
//         responseBuffer_.consume(parserLength);

//         if (!task->entry_->httpResponseComplete_) {
//             doReadResponse();
//             return;
//         }

//         task->setPhase(ESClientRequestPhase::Done, "Success");
//         runningTask_.reset();
//         doWaitNextTask();
//     }

//     void doWaitNextTask() {
//         doWaitEvent();
//         doWaitSocket();
//     }

//     void doWaitEvent() {
//         eventStream_.async_read_some(notifyBufferWrapper_,
//                                      [this](const std::error_code &ec, std::size_t bytes_transferred) {
//                                          afterWaitEvent(ec);
//                                      });
//     }

//     void afterWaitEvent(const asio::error_code &ec) {
//         if (ec && ec != asio::error::operation_aborted) {
//             throw std::runtime_error(
//                     std::string("fatal error occurred while reading eventStream :").append(ec.message()));
//         }
//         if (stream_ != nullptr && stream_->is_open()) {
//             asio::error_code tempErrorCode{};
//             stream_->cancel(tempErrorCode);
//         }
//         doWork();
//     }

//     void doWaitSocket() {
//         if (stream_ == nullptr) {
//             return;
//         }

//         if (!stream_->is_open()) {
//             return;
//         }

//         stream_->async_read_some(notifyBufferWrapper_,
//                                  [this](const std::error_code &ec, std::size_t bytes_transferred) {
//                                      afterWaitSocket(ec, bytes_transferred);
//                                  });
//     }

//     void afterWaitSocket(const asio::error_code &ec, std::size_t) {
//         if (ec && ec == asio::error::operation_aborted) {
//             return;
//         }
//         stream_.reset(new asio::ip::tcp::socket(ioContext_));
//     }

//     void afterSocketFail(const std::string &message) {
//         auto task = runningTask_.lock();
//         if (task != nullptr) {
//             task->setPhase(Error, message);
//         }
//         runningTask_.reset();
//         stream_.reset(new asio::ip::tcp::socket(ioContext_));
//         doWaitNextTask();
//     }

// private:
//     std::string host_;
//     int port_;
//     asio::io_context &ioContext_;
//     std::queue<std::shared_ptr<ESClientRequest>> &pendingTaskQueue_;
//     asio::posix::stream_descriptor &eventStream_;
//     std::unique_ptr<asio::ip::tcp::socket> stream_{};
//     std::weak_ptr<ESClientRequest> runningTask_{};
//     http_parser_settings responseParserSetting_{};
//     asio::streambuf responseBuffer_{};
//     asio::mutable_buffer notifyBufferWrapper_{NotifyBuffer.begin(), NotifyBuffer.size()};
// };


// void ESClient::start() {
//     int eventFD_ = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
//     // int eventFD_ = eventfd(0, 0);//, EFD_CLOEXEC | EFD_NONBLOCK);
//     eventStream_.assign(eventFD_);
//     for (size_t i = 0; i < MAX_ES_CLIENT_SESSION_SIZE; ++i) {
//         auto session = std::make_shared<ESClientWorker>(ESHost, ESPort, ioContext_, pendingQueue_, eventStream_);
//         session->run();
//         sessionVector_.push_back(session);
//     }
//     thread_ = std::thread([this] { ioContext_.run(); });
//     thread_.detach();
// }
//
//TC_HttpResponse
//ESClient::postRequest(ESClientRequestMethod method, const std::string &url, const std::string &body) {
//    std::shared_ptr<ESClientRequest> task(new ESClientRequest());
//    task->entry_ = std::make_shared<ESClientRequestEntry>();
//    task->entry_->initHttpResponseParser();
//    switch (method) {
//        case ESClientRequestMethod::Post: {
//            task->entry_->httpRequestContent_ = buildPostRequest(url, body);
//        }
//            break;
//        case ESClientRequestMethod::Put: {
//            task->entry_->httpRequestContent_ = buildPutRequest(url, body);
//        }
//            break;
//        case ESClientRequestMethod::Get: {
//            task->entry_->httpRequestContent_ = buildGetRequest(url, body);
//        }
//            break;
//        default:
//            return nullptr;
//    }
//    ioContext_.post(
//            [this, task] {
//                pendingQueue_.push(task);
//                eventStream_.async_write_some(asio::buffer(NotifyBuffer.begin(), NotifyBuffer.size()),
//                                              [](const std::error_code ec, std::size_t bytes_transferred) {
//                                                  if (ec) {
//                                                      throw std::runtime_error(std::string(
//                                                              "fatal error occurred while writing  eventStream :").append(
//                                                              ec.message()));
//                                                  }
//                                              });
//            }
//    );
//    return task;
//}

shared_ptr<TC_HttpResponse> ESClient::postRequest(ESClientRequestMethod method, const std::string &url, const std::string &body)
{
	shared_ptr<TC_HttpRequest> request = std::make_shared<TC_HttpRequest>();
	switch (method) {
		case ESClientRequestMethod::Post: {
			request->setPostRequest(url, body);
		}
			break;
		case ESClientRequestMethod::Put: {
			request->setPutRequest(url, body);
		}
			break;
		default:
		case ESClientRequestMethod::Get: {
			request->setGetRequest(url);
		}
			break;
	}

	shared_ptr<TC_HttpResponse> response = std::make_shared<TC_HttpResponse>();

	_esPrx->http_call("es", request, response);

	return response;
}
//
//std::string ESClient::buildPostRequest(const std::string &url, const std::string &body) {
//    std::ostringstream strStream;
//    strStream << "POST " << url << " HTTP/1.1\r\n";
//    strStream << "Content-Length: " << body.size() << "\r\n";
//    strStream << "Host: " << ESHost << ":" << ESPort << "\r\n";
//    strStream << "Content-Type: application/x-ndjson\r\n";
//    strStream << "Connection: Keep-Alive\r\n";
//    strStream << "\r\n";
//    strStream << body;
//    std::string requestContent = strStream.str();
//    return requestContent;
//}
//
//std::string ESClient::buildPutRequest(const std::string &url, const std::string &body) {
//    std::ostringstream strStream;
//    strStream << "PUT " << url << " HTTP/1.1\r\n";
//    strStream << "Content-Length: " << body.size() << "\r\n";
//    strStream << "Host: " << ESHost << ":" << ESPort << "\r\n";
//    strStream << "Content-Type: application/json\r\n";
//    strStream << "Connection: Keep-Alive\r\n";
//    strStream << "\r\n";
//    strStream << body;
//    std::string requestContent = strStream.str();
//    return requestContent;
//}
//
//std::string ESClient::buildGetRequest(const std::string &url, const std::string &body) {
//    std::ostringstream strStream;
//    strStream << "GET " << url << " HTTP/1.1\r\n";
//    strStream << "Content-Length: " << body.size() << "\r\n";
//    strStream << "Host: " << ESHost << ":" << ESPort << "\r\n";
//    strStream << "Content-Type: application/json\r\n";
//    strStream << "Connection: Keep-Alive\r\n";
//    strStream << "\r\n";
//    strStream << body;
//    std::string requestContent = strStream.str();
//    return requestContent;
//}

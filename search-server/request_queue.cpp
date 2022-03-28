#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server)
    : search_server_(search_server)
    , no_result_request_count_(0)
    , current_time_(0) {
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query,
                                                   DocumentStatus status) {
    return RequestQueue::AddFindRequest(raw_query,
                                        [status](int document_id, DocumentStatus document_status, int rating) {
                                            return document_status == status;
    });
}

int RequestQueue::GetNoResultRequests() const {
    return no_result_request_count_;
}

void RequestQueue::AddRequest(int results_num) {
    ++current_time_;

    while (!requests_.empty() && min_in_day_ <= current_time_ - requests_.front().timestamp) {
        if (0 == requests_.front().results) {
            --no_result_request_count_;
        }
        requests_.pop_front();
    }

    requests_.push_back({current_time_, results_num});
    if (0 == results_num) {
        ++no_result_request_count_;
    }
}


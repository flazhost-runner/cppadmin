#pragma once
#include <vector>
#include <string>
#include <cmath>
#include <json/json.h>

struct PaginateMeta {
    int total{0};
    int page{1};
    int pageSize{10};
    int totalPages{1};
    bool hasPrev{false};
    bool hasNext{false};

    Json::Value toJson() const {
        Json::Value j;
        j["total"]      = total;
        j["page"]       = page;
        j["pageSize"]   = pageSize;
        j["totalPages"] = totalPages;
        j["hasPrev"]    = hasPrev;
        j["hasNext"]    = hasNext;
        Json::Value nums(Json::arrayValue);
        for (int n : pageNumbers()) nums.append(n);
        j["pageNumbers"] = nums;
        return j;
    }

    // Windowed page numbers for template rendering.
    // Produces: [1, -1(…), cur-2..cur+2, -1(…), last]  (-1 = ellipsis)
    std::vector<int> pageNumbers() const {
        std::vector<int> nums;
        if (totalPages <= 7) {
            for (int i = 1; i <= totalPages; ++i) nums.push_back(i);
            return nums;
        }
        nums.push_back(1);
        if (page > 4) nums.push_back(-1);
        int lo = std::max(2, page - 2);
        int hi = std::min(totalPages - 1, page + 2);
        for (int i = lo; i <= hi; ++i) nums.push_back(i);
        if (page < totalPages - 3) nums.push_back(-1);
        nums.push_back(totalPages);
        return nums;
    }
};

struct PaginateResult {
    std::vector<std::string> ids; // used internally; actual rows held by caller
    PaginateMeta meta;
};

// Compute meta from total count + filter params.
inline PaginateMeta makePaginateMeta(int total, int page, int pageSize) {
    PaginateMeta m;
    m.total      = total;
    m.page       = std::max(1, page);
    m.pageSize   = (pageSize > 0) ? pageSize : 10;
    m.totalPages = std::max(1, (int)std::ceil((double)total / m.pageSize));
    m.page       = std::min(m.page, m.totalPages);
    m.hasPrev    = m.page > 1;
    m.hasNext    = m.page < m.totalPages;
    return m;
}

inline int pageOffset(int page, int pageSize) {
    return (std::max(1, page) - 1) * pageSize;
}

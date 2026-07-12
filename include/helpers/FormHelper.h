#pragma once

#include <drogon/HttpRequest.h>
#include <memory>
#include <string>
#include <vector>

// Pembacaan field form yang benar untuk SEMUA enctype.
//
// Dua jebakan Drogon 1.8.x yang keduanya gagal SENYAP:
//
// 1. HttpRequest::getParameter() hanya mengurai query string dan body
//    application/x-www-form-urlencoded. Body multipart/form-data TIDAK diurai —
//    getParameter() mengembalikan "" untuk setiap field teks pada form ber-upload
//    (profil, setting, user).
//
// 2. drogon::MultiPartParser::parse() GAGAL (mengembalikan non-nol, tanpa mengurai
//    apa pun) bila body memuat part berkas dengan filename kosong:
//
//        Content-Disposition: form-data; name="logo"; filename=""
//
//    Padahal itulah yang SELALU dikirim browser saat <input type="file"> dibiarkan
//    kosong — yaitu kasus paling umum: user cuma mengubah nama, tidak memilih foto.
//    Akibatnya seluruh body gagal terurai, _csrf tak terbaca, dan form yang sah
//    ditolak 403. Terverifikasi: body identik TANPA part kosong → 302; DENGAN part
//    kosong → 403.
//
// Karena itu multipart diurai di sini, bukan lewat MultiPartParser. Hasilnya di-cache
// di atribut request supaya body hanya dibaca sekali per permintaan (CsrfFilter dan
// controller sama-sama memerlukannya).
namespace form {

struct Part {
    std::string name;
    std::string filename;  // hanya untuk part berkas
    std::string data;
    bool        isFile{false};  // true bila atribut filename ADA (walau kosong)
};

using Parts = std::vector<Part>;

namespace detail {

inline std::string boundaryOf(const std::string &contentType) {
    auto pos = contentType.find("boundary=");
    if (pos == std::string::npos) return {};
    std::string b = contentType.substr(pos + 9);
    // buang tanda kutip dan spasi/parameter setelahnya
    if (!b.empty() && b.front() == '"') {
        auto end = b.find('"', 1);
        return (end == std::string::npos) ? std::string{} : b.substr(1, end - 1);
    }
    auto semi = b.find(';');
    if (semi != std::string::npos) b = b.substr(0, semi);
    while (!b.empty() && (b.back() == ' ' || b.back() == '\r' || b.back() == '\n'))
        b.pop_back();
    return b;
}

// Ambil nilai atribut `attr="..."` dari header Content-Disposition.
// `present` dibedakan dari "nilainya kosong": filename="" HARUS terbaca sebagai
// part berkas yang kosong, bukan sebagai field teks.
inline std::string attrOf(const std::string &headers, const std::string &attr,
                          bool &present) {
    present = false;
    std::string needle = attr + "=\"";
    auto pos = headers.find(needle);
    if (pos == std::string::npos) return {};
    pos += needle.size();
    auto end = headers.find('"', pos);
    if (end == std::string::npos) return {};
    present = true;
    return headers.substr(pos, end - pos);
}

inline Parts parseMultipart(const std::string &body, const std::string &boundary) {
    Parts out;
    const std::string delim = "--" + boundary;
    size_t pos = body.find(delim);
    if (pos == std::string::npos) return out;

    while (pos != std::string::npos) {
        pos += delim.size();
        // penutup: "--boundary--"
        if (body.compare(pos, 2, "--") == 0) break;

        auto headEnd = body.find("\r\n\r\n", pos);
        if (headEnd == std::string::npos) break;
        std::string headers = body.substr(pos, headEnd - pos);

        size_t dataStart = headEnd + 4;
        size_t next      = body.find("\r\n" + delim, dataStart);
        if (next == std::string::npos) break;

        Part p;
        bool hasName = false, hasFile = false;
        p.name     = attrOf(headers, "name", hasName);
        p.filename = attrOf(headers, "filename", hasFile);
        p.isFile   = hasFile;
        p.data     = body.substr(dataStart, next - dataStart);
        if (hasName) out.push_back(std::move(p));

        pos = next + 2;  // lompat ke delimiter berikutnya
    }
    return out;
}

// Urai sekali per request, simpan di atribut.
inline std::shared_ptr<Parts> partsOf(const drogon::HttpRequestPtr &req) {
    static const std::string kAttr = "form::parts";
    auto attrs = req->getAttributes();
    if (attrs->find(kAttr)) return attrs->get<std::shared_ptr<Parts>>(kAttr);

    auto parsed = std::make_shared<Parts>();
    const auto &ct = req->getHeader("content-type");
    if (ct.find("multipart/form-data") != std::string::npos) {
        auto b = boundaryOf(ct);
        if (!b.empty()) *parsed = parseMultipart(std::string(req->getBody()), b);
    }
    attrs->insert(kAttr, parsed);
    return parsed;
}

}  // namespace detail

inline bool isMultipart(const drogon::HttpRequestPtr &req) {
    return req->getHeader("content-type").find("multipart/form-data") != std::string::npos;
}

/// Nilai satu field teks, apa pun enctype-nya. "" bila tidak ada.
inline std::string get(const drogon::HttpRequestPtr &req, const std::string &key) {
    auto v = req->getParameter(key);  // query string + urlencoded
    if (!v.empty()) return v;
    if (!isMultipart(req)) return {};

    for (const auto &p : *detail::partsOf(req))
        if (!p.isFile && p.name == key) return p.data;
    return {};
}

/// Berkas terunggah pada field `key`. false bila tidak multipart, field tidak ada,
/// atau input file dibiarkan kosong (browser tetap mengirim part-nya dengan filename
/// dan isi kosong — itu BUKAN unggahan dan tidak boleh menimpa berkas lama).
inline bool file(const drogon::HttpRequestPtr &req,
                 const std::string           &key,
                 std::string                 &outData,
                 std::string                 &outFileName) {
    if (!isMultipart(req)) return false;

    for (const auto &p : *detail::partsOf(req)) {
        if (!p.isFile || p.name != key) continue;
        if (p.filename.empty() || p.data.empty()) continue;  // input dibiarkan kosong
        outData     = p.data;
        outFileName = p.filename;
        return true;
    }
    return false;
}

}  // namespace form

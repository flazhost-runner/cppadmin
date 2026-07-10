#pragma once
#include <drogon/HttpAppFramework.h>
#include <drogon/orm/DbClient.h>
#include <string>
#include <vector>

// Pengutipan identifier SQL per-dialek.
//
// Kolom `desc` (roles, permissions) adalah RESERVED WORD di MySQL, Postgres, dan
// SQLite. Drogon Mapper menyusun SQL dengan menyambung nama kolom apa adanya
// (`INSERT INTO roles (id,name,...,desc,...)`, `UPDATE roles SET desc = ?`),
// sehingga nama kolom harus sudah terkutip saat diserahkan ke Mapper.
//
// MySQL hanya menerima backtick (double-quote butuh sql_mode=ANSI_QUOTES, yang
// tidak bisa dipasang pada koneksi Drogon), sedangkan Postgres/SQLite memakai
// double-quote standar. Jadi kutipan dipilih dari tipe klien DB yang aktif.
namespace helpers {

inline std::string quoteIdent(const std::string &ident) {
    auto client = drogon::app().getDbClient();
    if (client && client->type() == drogon::orm::ClientType::Mysql)
        return "`" + ident + "`";
    return "\"" + ident + "\"";
}

inline std::vector<std::string> quoteIdents(const std::vector<std::string> &idents) {
    std::vector<std::string> out;
    out.reserve(idents.size());
    for (const auto &i : idents) out.push_back(quoteIdent(i));
    return out;
}

}  // namespace helpers

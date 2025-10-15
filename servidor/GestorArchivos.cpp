
#include "GestorArchivos.h"

#include <sstream>
#include <iostream>
#include <filesystem>
#include <ctime>
#include <cstdlib>
#include <algorithm>

using std::string;
namespace fs = std::filesystem;

static bool starts_with(const std::string& s, const std::string& pref) {
    return s.size() >= pref.size() && std::equal(pref.begin(), pref.end(), s.begin());
}

static bool ends_with(const std::string& s, const std::string& suf) {
    return s.size() >= suf.size() && std::equal(suf.rbegin(), suf.rend(), s.rbegin());
}

// Fecha/hora local
std::string GestorArchivos::nowAsString() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
}

std::string GestorArchivos::detectExtension(const std::string& path) {
    auto pos = path.find_last_of('.');
    if (pos == std::string::npos) return "";
    std::string ext = path.substr(pos + 1);
    // normalizar a minúsculas
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });
    return ext;
}

std::string GestorArchivos::trim(const std::string& s) {
    std::size_t i = 0, j = s.size();
    while (i < j && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    while (j > i && std::isspace(static_cast<unsigned char>(s[j-1]))) --j;
    return s.substr(i, j - i);
}

std::vector<std::string> GestorArchivos::split(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == delim) {
            out.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    out.push_back(cur);
    return out;
}

bool GestorArchivos::fileExists(const std::string& path) const {
    std::error_code ec;
    return fs::exists(path, ec);
}

void GestorArchivos::updateDimension() const {
    // La dejo const y uso ifstream local para no tocar el stream principal
    std::ifstream f(name_);
    if (!f) {
        const_cast<GestorArchivos*>(this)->dimension_ = 0;
        return;
    }
    std::size_t lines = 0;
    std::string dummy;
    while (std::getline(f, dummy)) ++lines;
    const_cast<GestorArchivos*>(this)->dimension_ = lines;
}

std::string GestorArchivos::readAll() const {
    std::ifstream f(name_, std::ios::in | std::ios::binary);
    if (!f) return "";
    std::ostringstream oss;
    oss << f.rdbuf();
    return oss.str();
}

// --------- Constructores / destructor ---------
GestorArchivos::GestorArchivos()
: name_(""), datetime_(nowAsString()), owner_(), dimension_(0), openMode_("r"), nextLineIdx_(0)
{
#ifdef _WIN32
    const char* u = std::getenv("USERNAME");
#else
    const char* u = std::getenv("USER");
#endif
    owner_ = (u ? u : "unknown");
}

GestorArchivos::GestorArchivos(const std::string& name)
: GestorArchivos()
{
    name_ = name;
    updateDimension();
}

GestorArchivos::GestorArchivos(const std::string& name, const std::string& datetime)
: GestorArchivos(name)
{
    datetime_ = datetime;
}

GestorArchivos::~GestorArchivos() {
    close();
}

// --------- Operaciones de apertura/cierre ---------
bool GestorArchivos::open(const std::string& mode) {
    close();
    openMode_ = mode;
    if (mode == "r") {
        fs_.open(name_, std::ios::in);
        nextLineIdx_ = 0;
    } else if (mode == "w") {
        fs_.open(name_, std::ios::out | std::ios::trunc);
    } else { // "a" (append) por defecto
        fs_.open(name_, std::ios::out | std::ios::app);
    }
    bool ok = fs_.is_open();
    if (ok) updateDimension();
    return ok;
}

void GestorArchivos::close() {
    if (fs_.is_open()) fs_.close();
}

// --------- Lectura de líneas ---------
std::string GestorArchivos::getLine() {
    if (!fs_.is_open() || openMode_ != "r") {
        open("r");
    }
    std::string line;
    if (std::getline(fs_, line)) {
        ++nextLineIdx_;
        return line;
    }
    return "";
}

std::string GestorArchivos::getLine(std::size_t idx) {
    std::ifstream f(name_);
    if (!f) return "";
    std::string line;
    std::size_t cur = 0;
    while (std::getline(f, line)) {
        ++cur;
        if (cur == idx) return line;
    }
    return "";
}

// --------- Escritura ---------
void GestorArchivos::write(const std::string& data) {
    if (!fs_.is_open() || (openMode_ != "w" && openMode_ != "a")) {
        open("a"); // por defecto agrego al final
    }
    fs_ << data;
    if (!data.empty() && data.back() != '\n') fs_ << '\n';
    fs_.flush();
    updateDimension();
}

// --------- Info y existencia ---------
bool GestorArchivos::exist() const {
    return fileExists(name_);
}

std::string GestorArchivos::getInfo() const {
    std::ostringstream oss;
    oss << "name: " << name_ << "\n";
    oss << "datetime: " << datetime_ << "\n";
    oss << "owner: " << owner_ << "\n";
    oss << "dimension: " << dimension_;
    return oss.str();
}

GestorArchivos::Table GestorArchivos::parseCSV(const std::string& text) const {
    std::istringstream iss(text);
    std::string line;
    std::vector<string> headers;
    std::vector<Row> rows;
    bool first = true;
    while (std::getline(iss, line)) {
        if (line.size() && (line.back()=='\r' || line.back()=='\n')) line.pop_back();
        if (trim(line).empty()) continue;
        auto parts = split(line, ',');
        if (first) {
            headers = parts;
            for (auto& h : headers) h = trim(h);
            first = false;
        } else {
            for (auto& p : parts) p = trim(p);
            rows.push_back(parts);
        }
    }
    return {headers, rows};
}

GestorArchivos::Table GestorArchivos::parseJSON(const std::string& text) const {
    std::string s = text;

    s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
    s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());

    std::vector<string> headers;
    std::vector<Row> rows;

    size_t pos = 0;
    while ((pos = s.find('{', pos)) != std::string::npos) {
        size_t end = s.find('}', pos);
        if (end == std::string::npos) break;
        std::string obj = s.substr(pos + 1, end - pos - 1);
        pos = end + 1;

        std::vector<std::pair<string,string>> pairs;
        std::string token;
        int braces = 0;
        bool inStr = false;
        for (size_t i = 0; i <= obj.size(); ++i) {
            char c = (i < obj.size() ? obj[i] : ',');
            if (c == '"' && (i == 0 || obj[i-1] != '\\')) inStr = !inStr;
            if (!inStr && braces == 0 && c == ',') {
                if (!trim(token).empty()) {
                    auto kv = split(token, ':');
                    if (kv.size() >= 2) {
                        string k = trim(kv[0]);
                        string v = trim(token.substr(token.find(':')+1));
                        if (k.size() >= 2 && k.front()=='"' && k.back()=='"') k = k.substr(1, k.size()-2);
                        if (v.size() >= 2 && v.front()=='"' && v.back()=='"') v = v.substr(1, v.size()-2);
                        pairs.push_back({k, v});
                    }
                }
                token.clear();
            } else {
                token.push_back(c);
            }
        }

        if (!pairs.empty()) {
            
            if (headers.empty()) {
                for (auto& p : pairs) headers.push_back(p.first);
            }
            
            Row r(headers.size(), "");
            for (auto& p : pairs) {
                auto it = std::find(headers.begin(), headers.end(), p.first);
                if (it != headers.end()) {
                    r[std::distance(headers.begin(), it)] = p.second;
                }
            }
            rows.push_back(r);
        }
    }
    return {headers, rows};
}


GestorArchivos::Table GestorArchivos::parseXML(const std::string& text) const {
    std::vector<string> headers;
    std::vector<Row> rows;

    std::string s = text;
    s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
    s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());

    size_t pos = 0;
    while ((pos = s.find("<row>", pos)) != std::string::npos) {
        size_t endRow = s.find("</row>", pos);
        if (endRow == std::string::npos) break;
        std::string rowBlock = s.substr(pos + 5, endRow - (pos + 5)); 
        pos = endRow + 6;

        size_t p2 = 0;
        std::vector<std::pair<string,string>> cols;
        while ((p2 = rowBlock.find("<col", p2)) != std::string::npos) {
            size_t namePos = rowBlock.find("name=\"", p2);
            if (namePos == std::string::npos) break;
            namePos += 6;
            size_t nameEnd = rowBlock.find("\"", namePos);
            if (nameEnd == std::string::npos) break;
            string header = rowBlock.substr(namePos, nameEnd - namePos);

            size_t gt = rowBlock.find(">", nameEnd);
            if (gt == std::string::npos) break;
            size_t colEnd = rowBlock.find("</col>", gt);
            if (colEnd == std::string::npos) break;
            string value = rowBlock.substr(gt + 1, colEnd - gt - 1);

            cols.push_back({header, value});
            p2 = colEnd + 6;
        }

        if (!cols.empty()) {
            if (headers.empty()) {
                for (auto& c : cols) headers.push_back(c.first);
            }
            Row r(headers.size(), "");
            for (auto& c : cols) {
                auto it = std::find(headers.begin(), headers.end(), c.first);
                if (it != headers.end()) {
                    r[std::distance(headers.begin(), it)] = c.second;
                }
            }
            rows.push_back(r);
        }
    }
    return {headers, rows};
}

std::string GestorArchivos::tableToCSV(const Table& t) {
    std::ostringstream oss;
    // headers
    for (std::size_t i = 0; i < t.first.size(); ++i) {
        if (i) oss << ",";
        oss << t.first[i];
    }
    oss << "\n";
    // filas
    for (const Row& r : t.second) {
        for (std::size_t i = 0; i < r.size(); ++i) {
            if (i) oss << ",";
            oss << r[i];
        }
        oss << "\n";
    }
    return oss.str();
}

std::string GestorArchivos::tableToJSON(const Table& t) {
    std::ostringstream oss;
    oss << "[\n";
    for (std::size_t r = 0; r < t.second.size(); ++r) {
        oss << "  {";
        for (std::size_t c = 0; c < t.first.size(); ++c) {
            if (c) oss << ", ";
            oss << "\"" << t.first[c] << "\":\"" << (c < t.second[r].size() ? t.second[r][c] : "") << "\"";
        }
        oss << "}";
        if (r + 1 < t.second.size()) oss << ",";
        oss << "\n";
    }
    oss << "]";
    return oss.str();
}

std::string GestorArchivos::tableToXML(const Table& t) {
    std::ostringstream oss;
    oss << "<rows>\n";
    for (const Row& r : t.second) {
        oss << "  <row>\n";
        for (std::size_t c = 0; c < t.first.size(); ++c) {
            oss << "    <col name=\"" << t.first[c] << "\">"
                << (c < r.size() ? r[c] : "") << "</col>\n";
        }
        oss << "  </row>\n";
    }
    oss << "</rows>";
    return oss.str();
}

GestorArchivos::Table GestorArchivos::readAsTable() const {
    std::string ext = detectExtension(name_);
    std::string text = readAll();
    if (ext == "csv")  return parseCSV(text);
    if (ext == "json") return parseJSON(text);
    if (ext == "xml")  return parseXML(text);
    
    std::string t = trim(text);
    if (!t.empty() && t.front() == '[') return parseJSON(text);
    if (!t.empty() && t.front() == '<') return parseXML(text);
    return parseCSV(text); 
}

std::string GestorArchivos::getCsv()  { return tableToCSV (readAsTable()); }
std::string GestorArchivos::getJson() { return tableToJSON(readAsTable()); }
std::string GestorArchivos::getXml()  { return tableToXML (readAsTable()); }

std::string GestorArchivos::buildPathCSV(const std::string& baseName) {
    namespace fs = std::filesystem;
    std::string dir = "data";
    std::error_code ec;
    if (!fs::exists(dir, ec)) fs::create_directories(dir, ec);

    std::string name = baseName;
    auto pos = name.find_last_of('.');
    if (pos != std::string::npos) name = name.substr(0, pos);
    return dir + "/" + name + ".csv";
}

// Parsea un registro devuelto por Arduino (c=csv, j=json, x=xml) a vector<string> de 4 campos
std::vector<std::string> GestorArchivos::parseRegistroComoVector(const std::string& texto, char formato) {
    std::vector<std::string> v(4);
    char f = std::tolower(static_cast<unsigned char>(formato));

    if (f == 'c') {
        auto parts = split(texto, ',');            // usa helper privado
        for (size_t i = 0; i < parts.size() && i < 4; ++i) {
            parts[i] = trim(parts[i]);             // usa helper privado
            v[i] = parts[i];
        }
    } else if (f == 'j') {
        auto findValue = [&](const std::string& key) -> std::string {
            size_t k = texto.find("\"" + key + "\"");
            if (k == std::string::npos) return "";
            size_t colon = texto.find(':', k);
            if (colon == std::string::npos) return "";
            size_t start = texto.find_first_not_of(" \t", colon + 1);
            if (start == std::string::npos) return "";
            if (texto[start] == '"') {
                size_t endq = texto.find('"', start + 1);
                if (endq == std::string::npos) return "";
                return texto.substr(start + 1, endq - (start + 1));
            } else {
                size_t endp = texto.find_first_of(",}\n\r", start);
                std::string num = texto.substr(start, endp - start);
                return trim(num);
            }
        };
        v[0] = findValue("dispositivo_id");
        v[1] = findValue("porcentaje_valvula");
        v[2] = findValue("estado_nivel");
        v[3] = findValue("caudal");
    } else if (f == 'x') {
        auto getTag = [&](const std::string& tag) -> std::string {
            std::string open = "<" + tag + ">";
            std::string close = "</" + tag + ">";
            size_t a = texto.find(open);
            size_t b = texto.find(close);
            if (a == std::string::npos || b == std::string::npos || b < a) return "";
            a += open.size();
            return trim(texto.substr(a, b - a));
        };
        v[0] = getTag("dispositivo_id");
        v[1] = getTag("porcentaje_valvula");
        v[2] = getTag("estado_nivel");
        v[3] = getTag("caudal");
    }
    return v;
}

// Construye un CSV con encabezado fijo del TP a partir de filas
std::string GestorArchivos::construirCSV(const std::vector<std::vector<std::string>>& filas) {
    std::ostringstream oss;
    oss << "dispositivo_id,porcentaje_valvula,estado_nivel,caudal\n";
    for (const auto& r : filas) {
        for (size_t i = 0; i < r.size(); ++i) {
            if (i) oss << ",";
            oss << r[i];
        }
        oss << "\n";
    }
    return oss.str();
}

// Imprime una tabla alineada (hasta N filas) a un ostream (por defecto cout)
void GestorArchivos::imprimirTablaCSV(const std::string& csv, std::size_t N, std::ostream& os) {
    std::istringstream iss(csv);
    std::string line;
    std::vector<std::vector<std::string>> rows;

    while (std::getline(iss, line)) {
        if (trim(line).empty()) continue;
        rows.push_back(split(line, ',')); // usa split privado
        for (auto& s : rows.back()) s = trim(s);
    }
    if (rows.empty()) return;

    std::vector<std::size_t> w(rows[0].size(), 0);
    auto considerar = [&](const std::vector<std::string>& r) {
        for (std::size_t i = 0; i < r.size(); ++i)
            w[i] = std::max<std::size_t>(w[i], r[i].size());
    };
    considerar(rows[0]);
    for (std::size_t i = 1; i < rows.size() && i <= N; ++i) considerar(rows[i]);

    auto printRow = [&](const std::vector<std::string>& r) {
        for (std::size_t i = 0; i < r.size(); ++i) {
            os << r[i];
            if (r[i].size() < w[i]) os << std::string(w[i] - r[i].size(), ' ');
            if (i + 1 < r.size()) os << "  |  ";
        }
        os << "\n";
    };

    // header + separador
    printRow(rows[0]);
    for (std::size_t i = 0; i < w.size(); ++i) {
        os << std::string(w[i], '-') << (i + 1 < w.size() ? "-----" : "");
    }
    os << "\n";

    // filas
    std::size_t printed = 0;
    for (std::size_t i = 1; i < rows.size() && printed < N; ++i, ++printed) {
        printRow(rows[i]);
    }
}

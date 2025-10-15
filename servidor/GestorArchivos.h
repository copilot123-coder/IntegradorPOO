
#ifndef GESTORARCHIVOS_H
#define GESTORARCHIVOS_H

#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <ostream>
#include <iostream>

class GestorArchivos {
private:
    std::string name_;        // nombre del archivo 
    std::string datetime_;    // fecha/hora
    std::string owner_;       // usuario 
    std::size_t dimension_;   // cantidad de líneas del archivo

    std::fstream fs_;         
    std::string openMode_;    
    std::size_t nextLineIdx_; 

    static std::string nowAsString();
    static std::string detectExtension(const std::string& path);
    static std::string trim(const std::string& s);
    static std::vector<std::string> split(const std::string& s, char delim);

    bool fileExists(const std::string& path) const;
    void updateDimension() const; 
    std::string readAll() const;  

    using Row   = std::vector<std::string>;
    using Table = std::pair<std::vector<std::string>, std::vector<Row>>;

    Table parseCSV(const std::string& text) const;
    Table parseJSON(const std::string& text) const;
    Table parseXML (const std::string& text) const;

    static std::string tableToCSV (const Table& t);
    static std::string tableToJSON(const Table& t);
    static std::string tableToXML (const Table& t);

    Table readAsTable() const;

public:
    // Constructores pedidos
    GestorArchivos();                                   // vacío
    explicit GestorArchivos(const std::string& name);   // con nombre
    GestorArchivos(const std::string& name, const std::string& datetime); // sólo nombre y fecha

    // Operaciones principales
    bool open(const std::string& mode = "r"); 
    void close();

    std::string getCsv();   // leer el archivo y devolverlo como CSV
    std::string getJson();  // leer el archivo y devolverlo como JSON
    std::string getXml();   // leer el archivo y devolverlo como XML

    std::string getLine();                 // siguiente línea disponible (modo lectura)
    std::string getLine(std::size_t idx);  // línea N. Devuelve "" si no existe.

    void write(const std::string& data);   // escribe al archivo

    bool exist() const;                    

    std::string getName() const        { return name_; }
    std::string getDatetime() const    { return datetime_; }
    std::string getOwner() const       { return owner_; }
    std::size_t getDimension() const   { return dimension_; }

    std::string getInfo() const;

    static std::string buildPathCSV(const std::string& baseName);
    static std::vector<std::string> parseRegistroComoVector(const std::string& texto, char formato);
    static std::string construirCSV(const std::vector<std::vector<std::string>>& filas);
    static void imprimirTablaCSV(const std::string& csv, std::size_t N, std::ostream& os = std::cout);

    ~GestorArchivos();
};

#endif

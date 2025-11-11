#include <gtest/gtest.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include "App.h"
#include "File.h"

using namespace Tp2_Act_1;

TEST(App, MostrarArchivoCSV) {
    // create temporary file
    const std::string tmp = "test_tmp.csv";
    std::ofstream ofs(tmp);
    ofs << "Id Dispositivo,Val1,Val2\n1 10 20\n";
    ofs.close();

    std::fstream fs(tmp, std::ios::in | std::ios::out);
    // construct File; pass nullptr for Trans_Rec since Mostrar_Archivo doesn't use it
    File file(fs, tmp, 'c', nullptr);

    App app(&file, nullptr);

    // capture stdout
    std::stringstream buffer;
    std::streambuf *sbuf = std::cout.rdbuf();
    std::cout.rdbuf(buffer.rdbuf());

    app.Mostrar_Archivo();

    std::cout.rdbuf(sbuf);
    std::string out = buffer.str();
    EXPECT_NE(out.find("[CSV]"), std::string::npos);

    // cleanup
    fs.close();
    std::remove(tmp.c_str());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

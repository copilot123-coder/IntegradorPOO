#include <gtest/gtest.h>
#include <fstream>
#include "GestorReportes.h"

using namespace std;

TEST(GestorReportes, ReporteGeneralEmptyFiles) {
    // Ensure files exist and are empty
    ofstream users("users.csv"); users << ""; users.close();
    ofstream logs("logs.csv"); logs << ""; logs.close();

    string out = GestorReportes::ReporteGeneral();
    EXPECT_NE(out.find("total_usuarios,"), string::npos);
    EXPECT_NE(out.find("total_logs,"), string::npos);
}

TEST(GestorReportes, ReporteAdminFilters) {
    ofstream users("users.csv");
    users << "1,alice,normal,1\n2,bob,admin,1\n";
    users.close();
    string out = GestorReportes::ReporteAdmin(2);
    EXPECT_NE(out.find("usuarios_asociados"), string::npos);
    EXPECT_NE(out.find("bob"), string::npos);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

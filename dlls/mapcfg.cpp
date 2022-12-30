#include <cstdio.h>
#include <string.h>

// map ismini tutan değişken
std::string mapName;

// map ismini alan fonksiyon
void GetMapName()
{
    char buffer[256];
    engine->pfnGetLevelName(buffer, sizeof(buffer)); // map ismini al
    mapName = buffer; // map ismini string'e ata
}

// map ismine göre cfg dosyasını çalıştıran fonksiyon
void ExecMapCfg()
{
    char command[256];
    sprintf(command, "exec %s.cfg", mapName.c_str()); // komutu oluştur
    engine->pfnServerCommand(command); // komutu çalıştır
}

// map değiştiğinde çalıştırılacak fonksiyon
void ChangeLevel()
{
    GetMapName(); // map ismini al
    ExecMapCfg(); // map ismine göre cfg dosyasını çalıştır
}

// oyun başladığında çalıştırılacak fonksiyon
void GameInit()
{
    // map değiştiğinde ChangeLevel fonksiyonunu çağıran komutu ekle
    engine->pfnAddServerCommand("changelevel", ChangeLevel);
}

// oyun sona erdiğinde çalıştırılacak fonksiyon
void Shutdown()
{
    // changelevel komutunu kaldır
    engine->pfnRemoveServerCommand("changelevel");
}

// C++ modülü oluştur
C_DLLEXPORT int GetEntityAPI2(DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion)
{
    // modül fonksiyonlarını doldur
    pFunctionTable->pfnGameInit = GameInit;
    pFunctionTable->pfnShutdown = Shutdown;
    return 1;
}

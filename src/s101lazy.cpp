#include "ogrsf_frmts.h"
#include "cpl_conv.h"
#include "S101GML.h"
#include <mutex>

std::mutex mtx;

class OGRS101LazyDriver: public GDALDriver{
    public:
        inline static const char *NAME = "S101Lazy";
        OGRS101LazyDriver(){
            SetDescription(NAME);
            SetMetadataItem(GDAL_DCAP_VECTOR, "YES");
            SetMetadataItem(GDAL_DMD_LONGNAME, "IHO S-101 lazy driver (using gml)");
            SetMetadataItem(GDAL_DMD_EXTENSION, "000");
            SetMetadataItem(GDAL_DCAP_VIRTUALIO, "YES");

            pfnOpen = Open;
            pfnIdentify = Identify;
            pfnCreate = nullptr; // READ ONLY
            
        }

        static GDALDataset *Open(GDALOpenInfo* poOpenInfo){
            if(poOpenInfo->eAccess == GA_Update){
                return nullptr; // READ ONLY
            }
            
            mtx.lock();
            libS101::S101 cell;
            cell.Open(poOpenInfo->pszFilename);
            std::string gmlData = S101GML::ExportToGML(cell);
            mtx.unlock();

            const char *tempGMLPath = "/vsimem/temp.gml";
            VSILFILE* fp = VSIFOpenL(tempGMLPath,"w");
            if(fp){
                VSIFWriteL(gmlData.c_str(),gmlData.size(),1,fp);
                VSIFCloseL(fp);
            }

            GDALDriver *gmlDriver = GetGDALDriverManager()->GetDriverByName("GML");
            if(gmlDriver && gmlDriver->pfnOpen){
                GDALOpenInfo oOpenInfo(tempGMLPath,GA_ReadOnly,nullptr);
                return gmlDriver->pfnOpen(&oOpenInfo);
            }

            return nullptr;
        }

        static int Identify(GDALOpenInfo* poOpenInfo){
            const char* pszFilename = poOpenInfo->pszFilename;
            if (EQUAL(CPLGetExtension(pszFilename), "000")) {
                return TRUE;
            }

            return FALSE;
        }

        static void Register(){
            if(GDALGetDriverByName(NAME)!=NULL)
                return;
            GDALDriver *poDriver = new OGRS101LazyDriver();
            GetGDALDriverManager()->RegisterDriver(poDriver);
        }
};

// Export the function properly
extern "C" void RegisterOGRS101Lazy()
{
    OGRS101LazyDriver::Register();
}
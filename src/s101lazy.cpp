#include "ogrsf_frmts.h"
#include "cpl_conv.h"
#include "S101GML.h"
#include <mutex>
#include <filesystem>


class OGRS101LazyDriver: public GDALDriver{
    public:
        inline static const char *NAME = "S101Lazy";
        inline static std::string cache_data;
        inline static std::time_t cache_time;
        inline static std::mutex mtx;

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
            auto mod_time = std::filesystem::last_write_time(poOpenInfo->pszFilename);
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                mod_time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
            );
            std::time_t mod_time_t = std::chrono::system_clock::to_time_t(sctp);

            if(mod_time_t!=cache_time){
                libS101::S101 cell;
                cell.Open(poOpenInfo->pszFilename);
                cache_data = S101GML::ExportToGML(cell);
                cache_time = mod_time_t;
            }
            mtx.unlock();

            const char *tempGMLPath = "/vsimem/temp.gml";
            VSILFILE* fp = VSIFOpenL(tempGMLPath,"w");
            if(fp){
                VSIFWriteL(cache_data.c_str(),cache_data.size(),1,fp);
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
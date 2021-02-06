#include <windows.h>
#include "rsl/dlmintf.h"
#include <string>
#include <fstream>      // std::ofstream
#include <time.h>
#include "mssup.h"

/*
typedef int emum {
    RSOEM=0,  // 0 RSOEM
    RSANSI, // 1 RSANSI
    LCOEM,  // 2 LCOEM
    LCANSI, // 3 LCANSI
    UTF8,   // 4 UTF-8
    UTF16LE,// 5 UTF-16LE
    UTF16BE // 6 UTF-16BE
} EncodeType;
*/

/* 
    RSLSrv передаёт в DLM строки в кодировке 1251 
    Если макрос может вызываться как из под "обычного" RSL 
    так и из под RSLSrv, то в RSL перед использованием надо дёрнуть 
    streamdocClaibrateLocaleA("А")

*/

int iRslCodePage=866;

static void streamdocClaibrateLocaleA(void) {
    VALUE *vStr;
    int ret=-1;

    if (GetParm (0,&vStr) && vStr->v_type == V_STRING) { 
        if(*(vStr->value.string)=='А') {
            ret=866;
        } else if(*(vStr->value.string)=='└') {
            iRslCodePage=ret=1251;
        }
    } else {
        RslError("Параметр №1 должен быть строкой с буквой А");
    }
    ReturnVal(V_INTEGER,(void *)&ret);
}


class  TStreamDoc {


public:

   TStreamDoc (TGenObject *pThis = NULL) {
      }

    ~TStreamDoc () {
        if(file) 
            fclose(file);
     
    }


   RSL_CLASS(TStreamDoc)

   // TStreamDoc (filename: string [, openmode: string [, encode: string]])
   RSL_INIT_DECL() {
      int N=GetNumParm();
      VALUE *vFileName;
      VALUE *vOpenMode;
      VALUE *vEncoding;

      if (GetParm(*firstParmOffs+0, &vFileName) && vFileName->v_type==V_STRING) {
          strFileName=vFileName->value.string;
      }

      if (GetParm(*firstParmOffs+1, &vOpenMode) && vOpenMode->v_type==V_STRING) {
          cOpenModeRS=vOpenMode->value.string[0];
          ConvertOpenmode();
      }

      if (GetParm(*firstParmOffs+2, &vEncoding) && vEncoding->v_type==V_STRING) {
          strEncoding=vEncoding->value.string;
          ConvertEncoding();
      }

      if(!strFileName.empty()) {
          file=fopen(strFileName.c_str(),sOpenmode);
      }

    }
                  
    RSL_METHOD_DECL(WriteLine) {

        VALUE *vVal;

        if (GetParm (1,  &vVal)) {
                switch (vVal->v_type) {
                case V_STRING:
                    if (file) {
                        if (866==encodingCodePage || CP_OEMCP==encodingCodePage) {
                            fputs(vVal->value.string, file);
                            fputs("\r\n", file);
                        } else {
                            char * s=ConvertFromCp866(vVal->value.string);
                            if (s) {
                                fputs(s, file);
                                fputs("\r\n", file);
                                free (s);
                            }
                        }
                    } // file
                } //switch
            }


        return 0;

    }



private:
    std::string strFileName;
    //std::string strOpenmode;
    char cOpenModeRS;
    std::string strEncoding; // принятое значение
    //rsencode_t encoding=RSOEM;     // внутреннее значение
    unsigned int encodingCodePage=866;
    char * sOpenmode="r";
    FILE * file=NULL;

    
    void ConvertOpenmode(void) { // from RS to C
        switch (cOpenModeRS) {
        case 'R': // ? открыть файл на чтение. Файл должен существовать.
            sOpenmode="r";
            break;
            
        case 'C': //  ? создать новый файл и открыть его на запись. Если файл уже существует, он перезаписывается.
        case 'W': //  ? эквивалентно C.
            sOpenmode="w";
            break;
        case 'A': //  ? открыть файл на дозапись. Если файл не существует, он создается.        }
            sOpenmode="a";
            break;
        default:
            sOpenmode="r";
        } //switch

    }

    void ConvertEncoding(void) { // from RS to C
        if ("rsoem"  ==strEncoding) {  encodingCodePage = CP_OEMCP; return;}                  
        if ("rsansi" ==strEncoding) {  encodingCodePage = CP_ACP;   return;}                  
        if ("lcansi" ==strEncoding) {  encodingCodePage = CP_ACP;   return;}                  
        if ("utf8"   ==strEncoding) {  encodingCodePage = CP_UTF8;  return;}                  
        if ("utf16le"==strEncoding) {  encodingCodePage = 1200;     return;} // not supported 
        if ("utf16be"==strEncoding) {  encodingCodePage = 1201;     return;} // not supported 

        encodingCodePage=866;  // default

    }




    // must free
    char * ConvertFromCp866(const char * s) {
        BSTR    bstrWide;
        char *  sRet=NULL;
        int     nLength;

        nLength = MultiByteToWideChar(866, 0, s, strlen(s) + 1, NULL, NULL);

        if (encodingCodePage == 1200 || encodingCodePage == 1201) {

        } else {
            bstrWide = SysAllocStringLen(NULL, nLength);

            MultiByteToWideChar(866, 0, s, strlen(s) + 1, bstrWide, nLength);

            nLength = WideCharToMultiByte(encodingCodePage, 0, bstrWide, -1, NULL, 0, NULL, NULL);
            sRet    = (char *)malloc(nLength+1);
            sRet[nLength]='\0';

            WideCharToMultiByte(encodingCodePage, 0, bstrWide, -1, sRet, nLength, NULL, NULL);
            SysFreeString(bstrWide);

        }

        return sRet;
    }

};

RSL_CLASS_BEGIN(TStreamDoc)
    RSL_METH       (WriteLine)
    RSL_INIT
RSL_CLASS_END


EXP32 void DLMAPI EXP AddModuleObjects (void) {
    // setlocale(LC_ALL,".866");
    RslAddUniClass (TStreamDoc::TablePtr,true);
    AddStdProc(V_INTEGER,"streamdocClaibrateLocaleA", streamdocClaibrateLocaleA, 0);
}

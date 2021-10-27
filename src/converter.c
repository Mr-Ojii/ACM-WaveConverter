//ref:https://eternalwindows.jp/winmm/mp3/mp305.html

#include <windows.h>
#include <mmreg.h>
#include <msacm.h>
#include <stdio.h>

int ErrorBox(const char *message)
{
    return MessageBox(NULL, message, "ACM-WaveConverter", MB_ICONERROR | MB_OK);
}

int main(int argc, char *argv[])
{
    if (argc <= 1)
        return 0;
    HMMIO hmmio = mmioOpen(argv[1], NULL, MMIO_READ);
    MMCKINFO mmckRiff;
    MMCKINFO mmckFmt;
    MMCKINFO mmckData;
    if (!hmmio)
    {
        ErrorBox("Failed to open the file.");
        return 0;
    }

    mmckRiff.fccType = mmioStringToFOURCC("WAVE", 0);
    if (!mmioDescend(hmmio, &mmckRiff, NULL, MMIO_FINDRIFF) == MMSYSERR_NOERROR)
    {
        ErrorBox("Not supported file.");
        mmioClose(hmmio, 0);
        return 0;
    }
    mmckFmt.ckid = mmioStringToFOURCC("fmt ", 0);
    if (!mmioDescend(hmmio, &mmckFmt, &mmckRiff, MMIO_FINDCHUNK) == MMSYSERR_NOERROR)
    {
        ErrorBox("Could not find \"fmt \" chunk.");
        mmioClose(hmmio, 0);
        return 0;
    }

    WAVEFORMATEX wfSrc;
    LPBYTE lpSrcData;
    DWORD dwSrcSize;
    mmioRead(hmmio, (HPSTR)&wfSrc, mmckFmt.cksize);
    mmioAscend(hmmio, &mmckFmt, 0);
    if (wfSrc.wFormatTag != WAVE_FORMAT_PCM)
    {
        ErrorBox("Not supported data format.");
        mmioClose(hmmio, 0);
        return 0;
    }

    mmckData.ckid = mmioStringToFOURCC("data", 0);
    mmioDescend(hmmio, &mmckData, &mmckRiff, MMIO_FINDCHUNK);
    lpSrcData = (LPBYTE)HeapAlloc(GetProcessHeap(), 0, mmckData.cksize);

    if (!lpSrcData)
    {
        ErrorBox("Could not allocate memory for SrcBuffer.");
        mmioClose(hmmio, 0);
        return 0;
    }

    mmioRead(hmmio, (HPSTR)lpSrcData, mmckData.cksize);
    mmioAscend(hmmio, &mmckData, 0);

    mmioAscend(hmmio, &mmckRiff, 0);
    mmioClose(hmmio, 0);

    dwSrcSize = mmckData.cksize;

    HACMSTREAM has;
    ACMSTREAMHEADER ash;
    LPBYTE lpDstData;
    DWORD dwDstSize;
    MMRESULT bResult;
    WAVEFORMATEX wfDst;

    wfDst.wFormatTag = WAVE_FORMAT_PCM;
    wfDst.nSamplesPerSec = 44100;
    wfDst.wBitsPerSample = 16;
    wfDst.nChannels = 2;
    wfDst.nBlockAlign = wfDst.nChannels * (wfDst.wBitsPerSample / 8);
    wfDst.nAvgBytesPerSec = wfDst.nSamplesPerSec * wfDst.nBlockAlign;
    wfDst.cbSize = 0;

    //4 == ACM_STREAMOPENF_NONREALTIME
    if ((bResult = acmStreamOpen(&has, NULL, &wfSrc, &wfDst, NULL, 0, 0, 4)) != 0)
    {
        char errormessage[260];
        sprintf(errormessage, "ErrorCode:%d\nFailed to acmStreamOpen.", bResult);
        ErrorBox(errormessage);
        HeapFree(GetProcessHeap(), 0, lpSrcData);
        return 0;
    }

    //0 == ACM_STREAMSIZEF_SOURCE
    acmStreamSize(has, dwSrcSize, &dwDstSize, 0);
    lpDstData = (LPBYTE)HeapAlloc(GetProcessHeap(), 0, dwDstSize);

    if (!lpDstData)
    {
        ErrorBox("Could not allocate memory for DstBuffer.");
        HeapFree(GetProcessHeap(), 0, lpSrcData);
        return 0;
    }

    ZeroMemory(&ash, sizeof(ACMSTREAMHEADER));
    ash.cbStruct = sizeof(ACMSTREAMHEADER);
    ash.pbSrc = lpSrcData;
    ash.cbSrcLength = dwSrcSize;
    ash.pbDst = lpDstData;
    ash.cbDstLength = dwDstSize;

    acmStreamPrepareHeader(has, &ash, 0);
    bResult = acmStreamConvert(has, &ash, 0);
    acmStreamUnprepareHeader(has, &ash, 0);

    if (bResult != 0)
    {
        char errormessage[MAX_PATH];
        sprintf(errormessage, "ErrorCode:%d\nFailed to acmStreamConvert.", bResult);
        ErrorBox(errormessage);
        HeapFree(GetProcessHeap(), 0, lpDstData);
        HeapFree(GetProcessHeap(), 0, lpSrcData);
        return 0;
    }

    HMMIO hmmioO;
    MMCKINFO mmckRiffO;
    MMCKINFO mmckFmtO;
    MMCKINFO mmckDataO;

    char DstWavName[MAX_PATH];
    char drive[MAX_PATH], dir[MAX_PATH], filename[MAX_PATH], ext[MAX_PATH];
    _splitpath(argv[1], drive, dir, filename, ext);
    strcpy(DstWavName, drive);
    strcat(DstWavName, dir);
    strcat(DstWavName, filename);
    strcat(DstWavName, "_2");
    strcat(DstWavName, ext);

    hmmioO = mmioOpen(DstWavName, NULL, MMIO_CREATE | MMIO_WRITE);

    mmckRiffO.fccType = mmioStringToFOURCC("WAVE", 0);
    mmioCreateChunk(hmmioO, &mmckRiffO, MMIO_CREATERIFF);

    mmckFmtO.ckid = mmioStringToFOURCC("fmt ", 0);
    mmioCreateChunk(hmmioO, &mmckFmtO, 0);
    mmioWrite(hmmioO, (char *)&wfDst, sizeof(WAVEFORMATEX));
    mmioAscend(hmmioO, &mmckFmtO, 0);

    mmckDataO.ckid = mmioStringToFOURCC("data", 0);
    mmioCreateChunk(hmmioO, &mmckDataO, 0);
    mmioWrite(hmmioO, (char *)lpDstData, dwDstSize);
    mmioAscend(hmmioO, &mmckDataO, 0);

    mmioAscend(hmmioO, &mmckRiffO, 0);
    mmioClose(hmmioO, 0);

    HeapFree(GetProcessHeap(), 0, lpDstData);
    HeapFree(GetProcessHeap(), 0, lpSrcData);

    MessageBox(NULL, "Completed.", "ACM-WaveConverter", MB_ICONINFORMATION | MB_OK);

    return 0;
}

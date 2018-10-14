class DaqL791: public LDaqBoard
{
public:
   // Base functions
      // Service functions
   IFC(ULONG) PlataTest();


   IFC(ULONG)  ReadFlashWord(USHORT Addr, PUSHORT Data);
   IFC(ULONG)  WriteFlashWord(USHORT Addr, USHORT Data);
   IFC(ULONG)  ReadPlataDescr(LPVOID pd);
   IFC(ULONG)  WritePlataDescr(LPVOID pd, USHORT Ena);
   IFC(ULONG)  EnableFlashWrite(USHORT Flag);   

public:
   DaqL791(ULONG Slot) :LDaqBoard(Slot) {}

   HANDLE csOpenLDevice();
   ULONG csCloseLDevice(ULONG status);
   ULONG csRequestBufferStream(ULONG *Size, ULONG StreamId, ULONG status); //in words
   ULONG csSetParametersStream(PDAQ_PAR sp, PULONG UsedSize, void** Data, void** Sync, ULONG StreamId, ULONG status);

   ULONG  FillADCparameters(PDAQ_PAR sp);
   ULONG  FillDACparameters(PDAQ_PAR sp);

   ULONG InputTTL(PDAQ_PAR sp);  //2 in 1 all
   ULONG OutputTTL(PDAQ_PAR sp);  // in each set channel
   ULONG ConfigTTL(PDAQ_PAR sp); // 1221 and 1450 780C 791
   ULONG OutputDAC(PDAQ_PAR sp); //2 in 1
   ULONG InputADC(PDAQ_PAR sp);

protected:
};

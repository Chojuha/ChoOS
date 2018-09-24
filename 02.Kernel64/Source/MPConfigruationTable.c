/**
Using Open Source.
copyright:kkamagui
*/

#include "MPConfigurationTable.h"
#include "Console.h"

static MPCONFIGURATIONMANAGER gs_stMPConfigurationManager = { 0, };

BOOL FindMPFloatingPointerAddress( CONSOLECOLOR Color , QWORD* pstAddress )
{
    char* pcMPFloatingPointer;
    QWORD qwEBDAAddress;
    QWORD qwSystemBaseMemory;
    qwEBDAAddress = *( WORD* ) ( 0x040E );
    qwEBDAAddress *= 16;
    
    for( pcMPFloatingPointer = ( char* ) qwEBDAAddress ; 
         ( QWORD ) pcMPFloatingPointer <= ( qwEBDAAddress + 1024 ) ; 
         pcMPFloatingPointer++ )
    {
        if( MemCmp( pcMPFloatingPointer, "_MP_", 4 ) == 0 )
        {
            *pstAddress = ( QWORD ) pcMPFloatingPointer;
            return TRUE;
        }
    }
    qwSystemBaseMemory = *( WORD* ) 0x0413;
    qwSystemBaseMemory *= 1024;

    for( pcMPFloatingPointer = ( char* ) ( qwSystemBaseMemory - 1024 ) ; 
        ( QWORD ) pcMPFloatingPointer <= qwSystemBaseMemory ; 
        pcMPFloatingPointer++ )
    {
        if( MemCmp( pcMPFloatingPointer, "_MP_", 4 ) == 0 )
        {
            *pstAddress = ( QWORD ) pcMPFloatingPointer;
            return TRUE;
        }
    }
    for( pcMPFloatingPointer = ( char* ) 0x0F0000; 
         ( QWORD) pcMPFloatingPointer < 0x0FFFFF; pcMPFloatingPointer++ )
    {
        if( MemCmp( pcMPFloatingPointer, "_MP_", 4 ) == 0 )
        {
            *pstAddress = ( QWORD ) pcMPFloatingPointer;
            return TRUE;
        }
    }

    return FALSE;
}

BOOL AnalysisMPConfigurationTable( CONSOLECOLOR Color )
{
    QWORD qwMPFloatingPointerAddress;
    MPFLOATINGPOINTER* MPFloatingPointer;
    MPCONFIGURATIONTABLEHEADER* pstMPConfigurationHeader;
    BYTE bEntryType;
    WORD i;
    QWORD qwEntryAddress;
    PROCESSORENTRY* pstProcessorEntry;
    BUSENTRY* pstBusEntry;
    MemSet( &gs_stMPConfigurationManager, 0, sizeof( MPCONFIGURATIONMANAGER ) );
    gs_stMPConfigurationManager.ISABusID = 0xFF;
    if( FindMPFloatingPointerAddress(NULL ,  &qwMPFloatingPointerAddress ) == FALSE )
    {
        return FALSE;
    }
    MPFloatingPointer = ( MPFLOATINGPOINTER* ) qwMPFloatingPointerAddress;
    gs_stMPConfigurationManager.MPFloatingPointer = MPFloatingPointer;
    pstMPConfigurationHeader = ( MPCONFIGURATIONTABLEHEADER* ) 
        ( ( QWORD ) MPFloatingPointer->MPConfigurationTableAddress & 0xFFFFFFFF );
    if( MPFloatingPointer->MPFeatureByte[ 1 ] & 
            MP_FLOATINGPOINTER_FEATUREBYTE2_PICMODE )
    {
        gs_stMPConfigurationManager.UsePICMode = TRUE;
    }
    gs_stMPConfigurationManager.MPConfigurationTableHeader = 
        pstMPConfigurationHeader;
    gs_stMPConfigurationManager.BaseEntryStartAddress = 
        MPFloatingPointer->MPConfigurationTableAddress + 
        sizeof( MPCONFIGURATIONTABLEHEADER );
    qwEntryAddress = gs_stMPConfigurationManager.BaseEntryStartAddress;
    for( i = 0 ; i < pstMPConfigurationHeader->EntryCount ; i++ )
    {
        bEntryType = *( BYTE* ) qwEntryAddress;
        switch( bEntryType )
        {
        case MP_ENTRYTYPE_PROCESSOR:
            pstProcessorEntry = ( PROCESSORENTRY* ) qwEntryAddress;
            if( pstProcessorEntry->CPUFlags & MP_PROCESSOR_CPUFLAGS_ENABLE )
            {
                gs_stMPConfigurationManager.ProcessorCount++;
            }
            qwEntryAddress += sizeof( PROCESSORENTRY );
            break;
        case MP_ENTRYTYPE_BUS:
            pstBusEntry = ( BUSENTRY* ) qwEntryAddress;
            if( MemCmp( pstBusEntry->BusTypeString, MP_BUS_TYPESTRING_ISA,
                    StrLen( MP_BUS_TYPESTRING_ISA ) ) == 0 )
            {
                gs_stMPConfigurationManager.ISABusID = pstBusEntry->BusID;
            }
            qwEntryAddress += sizeof( BUSENTRY );
            break;
        case MP_ENTRYTYPE_IOAPIC:
        case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
        case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
        default:
            qwEntryAddress += 8;
            break;
        }
    }
    return TRUE;
}

MPCONFIGURATIONMANAGER* GetMPConfigurationManager( void )
{
    return &gs_stMPConfigurationManager;
}

void PrintMPConfigurationTable( CONSOLECOLOR Color )
{
    MPCONFIGURATIONMANAGER* pstMPConfigurationManager;
    QWORD qwMPFloatingPointerAddress;
    MPFLOATINGPOINTER* MPFloatingPointer;
    MPCONFIGURATIONTABLEHEADER* pstMPTableHeader;
    PROCESSORENTRY* pstProcessorEntry;
    BUSENTRY* pstBusEntry;
    IOAPICENTRY* pstIOAPICEntry;
    IOINTERRUPTASSIGNMENTENTRY* pstIOAssignmentEntry;
    LOCALINTERRUPTASSIGNMENTENTRY* pstLocalAssignmentEntry;
    QWORD qwBaseEntryAddress;
    char vcStringBuffer[ 20 ];
    WORD i;
    BYTE bEntryType;
    char* vpcInterruptType[ 4 ] = { "INT", "NMI", "SMI", "ExtINT" };
    char* vpcInterruptFlagsPO[ 4 ] = { "Conform", "Active High", 
            "Reserved", "Active Low" };
    char* vpcInterruptFlagsEL[ 4 ] = { "Conform", "Edge-Trigger", "Reserved", 
            "Level-Trigger"};

    Printf(Color , "================ MP Configuration Table Summary ================\n" );
    pstMPConfigurationManager = GetMPConfigurationManager();
    if( ( pstMPConfigurationManager->BaseEntryStartAddress == 0 ) &&
        ( AnalysisMPConfigurationTable(Color) == FALSE ) )
    {
        Printf(Color , "MP Configuration Table Analysis Fail\n" );
        return ;
    }
    Printf(Color , "MP Configuration Table Analysis Success\n" );
    
    Printf(Color , "MP Floating Pointer Address.....................: 0x%Q\n", 
            pstMPConfigurationManager->MPFloatingPointer );
    Printf(Color , "PIC Mode Support................................: %d\n", pstMPConfigurationManager->UsePICMode );
    Printf(Color , "MP Configuration Table Header Address...........: 0x%Q\n",
            pstMPConfigurationManager->MPConfigurationTableHeader );
    Printf(Color , "Base MP Configuration Table Entry Start Address.: 0x%Q\n",
            pstMPConfigurationManager->BaseEntryStartAddress );
    Printf(Color , "Processor Count.................................: %d\n", pstMPConfigurationManager->ProcessorCount );
    Printf(Color , "ISA Bus ID......................................: %d\n", pstMPConfigurationManager->ISABusID );
	if(Paused(Color) == TRUE) {
		return;
	}
    Printf(Color , "=================== MP Floating Pointer ===================\n" );
    MPFloatingPointer = pstMPConfigurationManager->MPFloatingPointer;
    MemCpy( vcStringBuffer, MPFloatingPointer->Signature, 4 );
    vcStringBuffer[ 4 ] = '\0';
    Printf(Color , "Signature......................: %s\n", vcStringBuffer );
    Printf(Color , "MP Configuration Table Address.: 0x%Q\n", 
            MPFloatingPointer->MPConfigurationTableAddress );
    Printf(Color , "Length.........................: %d * 16 Byte\n", MPFloatingPointer->Length );
    Printf(Color , "Version........................: %d\n", MPFloatingPointer->Revision );
    Printf(Color , "CheckSum.......................: 0x%X\n", MPFloatingPointer->CheckSum );
    Printf(Color , "Feature Byte 1.................: 0x%X | ", MPFloatingPointer->MPFeatureByte[ 0 ] );
    if( MPFloatingPointer->MPFeatureByte[ 0 ] == 0 )
    {
        Printf(Color , "Use MP Configuration Table\n" );
    }
    else
    {
        Printf(Color , "Use Default Configuration\n" );
    }
    Printf(Color , "Feature Byte 2                 : 0x%X | ", MPFloatingPointer->MPFeatureByte[ 1 ] );
    if( MPFloatingPointer->MPFeatureByte[ 2 ] & 
            MP_FLOATINGPOINTER_FEATUREBYTE2_PICMODE )
    {
        Printf(Color , "PIC Mode Support\n" );
    }
    else
    {
        Printf(Color , "Virtual Wire Mode Support\n" );
    }
    Printf(Color , "\n=============== MP Configuration Table Header ===============\n" );
    pstMPTableHeader = pstMPConfigurationManager->MPConfigurationTableHeader;
    MemCpy( vcStringBuffer, pstMPTableHeader->Signature, 4 );
    vcStringBuffer[ 4 ] = '\0';
    Printf(Color , "Signature...............................: %s\n", vcStringBuffer );
    Printf(Color , "Length..................................: %d Byte\n", pstMPTableHeader->BaseTableLength );
    Printf(Color , "Version.................................: %d\n", pstMPTableHeader->Revision );
    Printf(Color , "CheckSum................................: 0x%X\n", pstMPTableHeader->CheckSum );
    MemCpy( vcStringBuffer, pstMPTableHeader->OEMIDString, 8 );
    vcStringBuffer[ 8 ] = '\0';
    Printf(Color , "OEM ID String...........................: %s\n", vcStringBuffer );
    MemCpy( vcStringBuffer, pstMPTableHeader->ProductIDString, 12 );
    vcStringBuffer[ 12 ] = '\0';
    Printf(Color , "Product ID String.......................: %s\n", vcStringBuffer );
    Printf(Color , "OEM Table Pointer.......................: 0x%X\n", 
             pstMPTableHeader->OEMTablePointerAddress );
    Printf(Color , "OEM Table Size..........................: %d Byte\n", pstMPTableHeader->OEMTableSize );
    Printf(Color , "Entry Count.............................: %d\n", pstMPTableHeader->EntryCount );
    Printf(Color , "Memory Mapped I/O Address Of Local APIC.: 0x%X\n",
            pstMPTableHeader->MemoryMapIOAddressOfLocalAPIC );
    Printf(Color , "Extended Table Length...................: %d Byte\n", 
            pstMPTableHeader->ExtendedTableLength );
    Printf(Color , "Extended Table Checksum.................: 0x%X\n", 
            pstMPTableHeader->ExtendedTableCheckSum );
    
    if(Paused(Color) == TRUE) {
    	return;
    }
    
    Printf(Color , "\n============= Base MP Configuration Table Entry =============\n" );
    qwBaseEntryAddress = MPFloatingPointer->MPConfigurationTableAddress + 
        sizeof( MPCONFIGURATIONTABLEHEADER );
    for( i = 0 ; i < pstMPTableHeader->EntryCount ; i++ )
    {
        bEntryType = *( BYTE* ) qwBaseEntryAddress;
        switch( bEntryType )
        {
        case MP_ENTRYTYPE_PROCESSOR:
            pstProcessorEntry = ( PROCESSORENTRY* ) qwBaseEntryAddress;
            Printf(Color , "Entry Type.........: Processor\n" );
            Printf(Color , "Local APIC ID......: %d\n", pstProcessorEntry->LocalAPICID );
            Printf(Color , "Local APIC Version.: 0x%X\n", pstProcessorEntry->LocalAPICVersion );
            Printf(Color , "CPU Flags..........: 0x%X | ", pstProcessorEntry->CPUFlags );
            if( pstProcessorEntry->CPUFlags & MP_PROCESSOR_CPUFLAGS_ENABLE )
            {
                Printf(Color , "Enable | " );
            }
            else
            {
                Printf(Color , "Disable | " );
            }
            if( pstProcessorEntry->CPUFlags & MP_PROCESSOR_CPUFLAGS_BSP )
            {
                Printf(Color , "BSP \n" );
            }
            else
            {
                Printf(Color , "AP \n" );
            }
            Printf(Color , "CPU Signature......: 0x%X\n", pstProcessorEntry->CPUSignature );
            Printf(Color , "Feature Flags......: 0x%X\n\n", pstProcessorEntry->FeatureFlags );
            qwBaseEntryAddress += sizeof( PROCESSORENTRY );
            break;
        case MP_ENTRYTYPE_BUS:
            pstBusEntry = ( BUSENTRY* ) qwBaseEntryAddress;
            Printf(Color , "Entry Type.........: Bus\n" );
            Printf(Color , "Bus ID.............: %d\n", pstBusEntry->BusID );
            MemCpy( vcStringBuffer, pstBusEntry->BusTypeString, 6 );
            vcStringBuffer[ 6 ] = '\0';
            Printf(Color , "Bus Type String....: %s\n\n", vcStringBuffer );
            qwBaseEntryAddress += sizeof( BUSENTRY );
            break;
        case MP_ENTRYTYPE_IOAPIC:
            pstIOAPICEntry = ( IOAPICENTRY* ) qwBaseEntryAddress;
            Printf(Color , "Entry Type................: I/O APIC\n" );
            Printf(Color , "I/O APIC ID...............: %d\n", pstIOAPICEntry->IOAPICID );
            Printf(Color , "I/O APIC Version..........: 0x%X\n", pstIOAPICEntry->IOAPICVersion );
            Printf(Color , "I/O APIC Flags............: 0x%X | ", pstIOAPICEntry->IOAPICFlags );
            if( pstIOAPICEntry->IOAPICFlags == 1 )
            {
                Printf(Color , "Enable\n" );
            }
            else
            {
                Printf(Color , "Disable\n" );
            }
            Printf(Color , "Memory Mapped I/O Address.: 0x%X\n\n", 
                    pstIOAPICEntry->MemoryMapAddress );
            qwBaseEntryAddress += sizeof( IOAPICENTRY );
            break;
        case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
            pstIOAssignmentEntry = ( IOINTERRUPTASSIGNMENTENTRY* ) 
                qwBaseEntryAddress;
            Printf(Color , "Entry Type.................: I/O Interrupt Assignment\n" );
            Printf(Color , "Interrupt Type.............: 0x%X ", pstIOAssignmentEntry->InterruptType );
            Printf(Color , "(%s)\n", vpcInterruptType[ pstIOAssignmentEntry->InterruptType ] );
            Printf(Color , "I/O Interrupt Flags........: 0x%X ", pstIOAssignmentEntry->InterruptFlags );
            Printf(Color , "(%s, %s)\n", vpcInterruptFlagsPO[ pstIOAssignmentEntry->InterruptFlags & 0x03 ], 
                    vpcInterruptFlagsEL[ ( pstIOAssignmentEntry->InterruptFlags >> 2 ) & 0x03 ] );
            Printf(Color , "Source BUS ID..............: %d\n", pstIOAssignmentEntry->SourceBUSID );
            Printf(Color , "Source BUS IRQ.............: %d\n", pstIOAssignmentEntry->SourceBUSIRQ );
            Printf(Color , "Destination I/O APIC ID....: %d\n", 
                     pstIOAssignmentEntry->DestinationIOAPICID );
            Printf(Color , "Destination I/O APIC INTIN.: %d\n\n", 
                     pstIOAssignmentEntry->DestinationIOAPICINTIN );
            qwBaseEntryAddress += sizeof( IOINTERRUPTASSIGNMENTENTRY );
            break;
        case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
            pstLocalAssignmentEntry = ( LOCALINTERRUPTASSIGNMENTENTRY* )
                qwBaseEntryAddress;
            Printf(Color , "Entry Type....................: Local Interrupt Assignment\n" );
            Printf(Color , "Interrupt Type : 0x%X ", pstLocalAssignmentEntry->InterruptType );
            Printf(Color , "| %s\n", vpcInterruptType[ pstLocalAssignmentEntry->InterruptType ] );
            Printf(Color , "I/O Interrupt Flags...........: 0x%X | ", pstLocalAssignmentEntry->InterruptFlags );
            Printf(Color , "%s | %s\n", vpcInterruptFlagsPO[ pstLocalAssignmentEntry->InterruptFlags & 0x03 ], 
                    vpcInterruptFlagsEL[ ( pstLocalAssignmentEntry->InterruptFlags >> 2 ) & 0x03 ] );
            Printf(Color , "Source BUS ID.................: %d\n", pstLocalAssignmentEntry->SourceBUSID );
            Printf(Color , "Source BUS IRQ................: %d\n", pstLocalAssignmentEntry->SourceBUSIRQ );
            Printf(Color , "Destination Local APIC ID.....: %d\n", 
                     pstLocalAssignmentEntry->DestinationIOAPICID );
            Printf(Color , "Destination Local APIC LINTIN.: %d\n\n", 
                     pstLocalAssignmentEntry->DestinationIOAPICINTIN );
            qwBaseEntryAddress += sizeof( LOCALINTERRUPTASSIGNMENTENTRY );
            break;
            
        default :
            Printf(Color , "Unknown Entry Type:%d\n", bEntryType );
            break;
        }
        if( ( i != 0 ) && ( ( ( i + 1 ) % 3 ) == 0 ) )
        {
            if(Paused(Color) == TRUE) {
				return;
			}
        }        
    }
}

IOAPICENTRY* FindIOAPICEntryForISA( void )
{
    MPCONFIGURATIONMANAGER* pstMPManager;
    MPCONFIGURATIONTABLEHEADER* pstMPHeader;
    IOINTERRUPTASSIGNMENTENTRY* pstIOAssignmentEntry;
    IOAPICENTRY* pstIOAPICEntry;
    QWORD qwEntryAddress;
    BYTE bEntryType;
    BOOL bFind = FALSE;
    int i;
    pstMPHeader = gs_stMPConfigurationManager.MPConfigurationTableHeader;
    qwEntryAddress = gs_stMPConfigurationManager.BaseEntryStartAddress;
    for( i = 0 ; ( i < pstMPHeader->EntryCount ) &&
                 ( bFind == FALSE ) ; i++ )
    {
        bEntryType = *( BYTE* ) qwEntryAddress;
        switch( bEntryType )
        {
        case MP_ENTRYTYPE_PROCESSOR:
            qwEntryAddress += sizeof( PROCESSORENTRY );
            break;
        case MP_ENTRYTYPE_BUS:
        case MP_ENTRYTYPE_IOAPIC:
        case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
            qwEntryAddress += 8;
            break;
        case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
            pstIOAssignmentEntry = ( IOINTERRUPTASSIGNMENTENTRY* ) qwEntryAddress;
            if( pstIOAssignmentEntry->SourceBUSID == 
                gs_stMPConfigurationManager.ISABusID )
            {
                bFind = TRUE;
            }                    
            qwEntryAddress += sizeof( IOINTERRUPTASSIGNMENTENTRY );
            break;
        }
    }
    if( bFind == FALSE )
    {
        return NULL;
    }
    qwEntryAddress = gs_stMPConfigurationManager.BaseEntryStartAddress;
    for( i = 0 ; i < pstMPHeader->EntryCount ; i++ )
    {
        bEntryType = *( BYTE* ) qwEntryAddress;
        switch( bEntryType ) {
        case MP_ENTRYTYPE_PROCESSOR:
            qwEntryAddress += sizeof( PROCESSORENTRY );
            break;
        case MP_ENTRYTYPE_BUS:
        case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
        case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
            qwEntryAddress += 8;
            break;
        case MP_ENTRYTYPE_IOAPIC:
            pstIOAPICEntry = ( IOAPICENTRY* ) qwEntryAddress;
            if( pstIOAPICEntry->IOAPICID == pstIOAssignmentEntry->DestinationIOAPICID )
            {
                return pstIOAPICEntry;
            }
            qwEntryAddress += sizeof( IOINTERRUPTASSIGNMENTENTRY );
            break;
        }
    }
    
    return NULL;
}

int GetProcessorCount( void )
{
    if( gs_stMPConfigurationManager.ProcessorCount == 0 )
    {
        return 1;
    }
    return gs_stMPConfigurationManager.ProcessorCount;
}

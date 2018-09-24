/**
Using Open Source.
copyright:kkamagui
*/

#include "IOAPIC.h"
#include "MPConfigurationTable.h"
#include "PIC.h"
#include "Colors.h"

static IOAPICMANAGER gs_stIOAPICManager;

QWORD GetIOAPICBaseAddressOfISA( void )
{
    MPCONFIGURATIONMANAGER* pstMPManager;
    IOAPICENTRY* pstIOAPICEntry;
    if( gs_stIOAPICManager.IOAPICBaseAddressOfISA == NULL )
    {
        pstIOAPICEntry = FindIOAPICEntryForISA();
        if( pstIOAPICEntry != NULL )
        {
            gs_stIOAPICManager.IOAPICBaseAddressOfISA = 
                pstIOAPICEntry->MemoryMapAddress & 0xFFFFFFFF;
        }
    }
    return gs_stIOAPICManager.IOAPICBaseAddressOfISA;
}

void SetIOAPICRedirectionEntry( IOREDIRECTIONTABLE* pstEntry, BYTE bAPICID,
        BYTE bInterruptMask, BYTE bFlagsAndDeliveryMode, BYTE bVector )
{
    MemSet( pstEntry, 0, sizeof( IOREDIRECTIONTABLE ) );
    
    pstEntry->Destination = bAPICID;
    pstEntry->FlagsAndDeliveryMode = bFlagsAndDeliveryMode;
    pstEntry->InterruptMask = bInterruptMask;
    pstEntry->Vector = bVector;
}

void ReadIOAPICRedirectionTable( int iINTIN, IOREDIRECTIONTABLE* pstEntry )
{
    QWORD* pqwData;
    QWORD qwIOAPICBaseAddress;
    qwIOAPICBaseAddress = GetIOAPICBaseAddressOfISA();
    pqwData = ( QWORD* ) pstEntry;
    *( DWORD* ) ( qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR ) = 
        IOAPIC_REGISTERINDEX_HIGHIOREDIRECTIONTABLE + iINTIN * 2;
    *pqwData = *( DWORD* ) ( qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW );
    *pqwData = *pqwData << 32;
    *( DWORD* ) ( qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR ) =
        IOAPIC_REGISTERINDEX_LOWIOREDIRECTIONTABLE + iINTIN * 2 ;
    *pqwData |= *( DWORD* ) ( qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW );
}

void WriteIOAPICRedirectionTable( int iINTIN, IOREDIRECTIONTABLE* pstEntry )
{
    QWORD* pqwData;
    QWORD qwIOAPICBaseAddress;
    qwIOAPICBaseAddress = GetIOAPICBaseAddressOfISA();
    pqwData = ( QWORD* ) pstEntry;
    *( DWORD* ) ( qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR ) = 
        IOAPIC_REGISTERINDEX_HIGHIOREDIRECTIONTABLE + iINTIN * 2;
    *( DWORD* ) ( qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW ) = *pqwData >> 32;
    *( DWORD* ) ( qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR ) =
        IOAPIC_REGISTERINDEX_LOWIOREDIRECTIONTABLE + iINTIN * 2 ;
    *( DWORD* ) ( qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW ) = *pqwData;
}

void MaskAllInterruptInIOAPIC( void )
{
    IOREDIRECTIONTABLE stEntry;
    int i;
    
    for( i = 0 ; i < IOAPIC_MAXIOREDIRECTIONTABLECOUNT ; i++ )
    {
        ReadIOAPICRedirectionTable( i, &stEntry );
        stEntry.InterruptMask = IOAPIC_INTERRUPT_MASK;
        WriteIOAPICRedirectionTable( i, &stEntry );
    }
}

void InitIORedirectionTable( void )
{
    MPCONFIGURATIONMANAGER* pstMPManager;
    MPCONFIGURATIONTABLEHEADER* pstMPHeader;
    IOINTERRUPTASSIGNMENTENTRY* pstIOAssignmentEntry;
    IOREDIRECTIONTABLE stIORedirectionEntry;
    QWORD qwEntryAddress;
    BYTE bEntryType;
    BYTE bDestination;
    int i;

    //==========================================================================
    // I/O APIC�� �����ϴ� �ڷᱸ���� �ʱ�ȭ
    //==========================================================================
    MemSet( &gs_stIOAPICManager, 0, sizeof( gs_stIOAPICManager ) );
    
    // I/O APIC�� �޸� �� I/O ��巹�� ����, �Ʒ� �Լ����� ���������� ó����
    GetIOAPICBaseAddressOfISA();
    
    // IRQ�� I/O APIC�� INTIN �ɰ� ������ ���̺�(IRQ->INTIN ���� ���̺�)�� �ʱ�ȭ
    for( i = 0 ; i < IOAPIC_MAXIRQTOINTINMAPCOUNT ; i++ )
    {
        gs_stIOAPICManager.IRQToINTINMap[ i ] = 0xFF;
    }
    
    //==========================================================================
    // I/O APIC�� ����ũ�Ͽ� ���ͷ�Ʈ�� �߻����� �ʵ��� �ϰ� I/O �����̷��� ���̺� �ʱ�ȭ
    //==========================================================================
    // ���� I/O APIC�� ���ͷ�Ʈ�� ����ũ�Ͽ� ���ͷ�Ʈ�� �߻����� �ʵ��� ��
    MaskAllInterruptInIOAPIC();
    
    // IO ���ͷ�Ʈ ���� ��Ʈ�� �߿��� ISA ������ ���õ� ���ͷ�Ʈ�� �߷��� I/O �����̷���
    // ���̺� ����
    // MP ���� ���̺� ����� ���� ��巹���� ��Ʈ���� ���� ��巹���� ����
    pstMPManager = GetMPConfigurationManager();
    pstMPHeader = pstMPManager->MPConfigurationTableHeader;
    qwEntryAddress = pstMPManager->BaseEntryStartAddress;
    
    // ��� ��Ʈ���� Ȯ���Ͽ� ISA ������ ���õ� I/O ���ͷ�Ʈ ���� ��Ʈ���� �˻�
    for( i = 0 ; i < pstMPHeader->EntryCount ; i++ )
    {
        bEntryType = *( BYTE* ) qwEntryAddress;
        switch( bEntryType )
        {
            // IO ���ͷ�Ʈ ���� ��Ʈ���̸�, ISA �������� Ȯ���Ͽ� I/O �����̷���
            // ���̺� �����ϰ� IRQ->INITIN ���� ���̺��� ����
        case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
            pstIOAssignmentEntry = ( IOINTERRUPTASSIGNMENTENTRY* ) qwEntryAddress;

            // ���ͷ�Ʈ Ÿ���� ���ͷ�Ʈ(INT)�� �͸� ó��
            if( ( pstIOAssignmentEntry->SourceBUSID == pstMPManager->ISABusID ) &&
                ( pstIOAssignmentEntry->InterruptType == MP_INTERRUPTTYPE_INT ) )                        
            {
                // ������ �ʵ�� IRQ 0�� �����ϰ� 0x00���� �����Ͽ� Bootstrap Processor�� ����
                // IRQ 0�� �����ٷ��� ����ؾ� �ϹǷ� 0xFF�� �����Ͽ� ��� �ھ�� ����
                if( pstIOAssignmentEntry->SourceBUSIRQ == 0 )
                {
                    bDestination = 0xFF;
                }
                else
                {
                    bDestination = 0x00;
                }
                
                // ISA ������ ���� Ʈ����(Edge Trigger)�� 1�� �� Ȱ��ȭ(Active High)��
                // ���
                // ������ ���� ���� ���, ���� ���� ����(Fixed)���� �Ҵ�
                // ���ͷ�Ʈ ���ʹ� PIC ��Ʈ�ѷ��� ���Ϳ� ���� 0x20 + IRQ�� ����
                SetIOAPICRedirectionEntry( &stIORedirectionEntry, bDestination, 
                    0x00, IOAPIC_TRIGGERMODE_EDGE | IOAPIC_POLARITY_ACTIVEHIGH |
                    IOAPIC_DESTINATIONMODE_PHYSICALMODE | IOAPIC_DELIVERYMODE_FIXED, 
                    PIC_IRQSTARTVECTOR + pstIOAssignmentEntry->SourceBUSIRQ );
                
                // ISA �������� ���޵� IRQ�� I/O APIC�� INTIN �ɿ� �����Ƿ�, INTIN ����
                // �̿��Ͽ� ó��
                WriteIOAPICRedirectionTable( pstIOAssignmentEntry->DestinationIOAPICINTIN, 
                        &stIORedirectionEntry );
                
                // IRQ�� ���ͷ�Ʈ �Է� ��(INTIN)�� ���踦 ����(IRQ->INTIN ���� ���̺� ����)
                gs_stIOAPICManager.IRQToINTINMap[ pstIOAssignmentEntry->SourceBUSIRQ ] =
                    pstIOAssignmentEntry->DestinationIOAPICINTIN;                
            }                    
            qwEntryAddress += sizeof( IOINTERRUPTASSIGNMENTENTRY );
            break;
        
            // ���μ��� ��Ʈ���� ����
        case MP_ENTRYTYPE_PROCESSOR:
            qwEntryAddress += sizeof( PROCESSORENTRY );
            break;
            
            // ���� ��Ʈ��, I/O APIC ��Ʈ��, ���� ���ͷ�Ʈ ���� ��Ʈ���� ����
        case MP_ENTRYTYPE_BUS:
        case MP_ENTRYTYPE_IOAPIC:
        case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
            qwEntryAddress += 8;
            break;
        }
    }  
}

/**
 *  IRQ�� I/O APIC�� ���ͷ�Ʈ ��(INT IN)���� ���� ���踦 ���
 */
void PrintIRQToINTINMap( CONSOLECOLOR Color )
{
    int i;
    
    Printf(0x17 , "===========IRQ To I/O APIC INT IN Mapping Table===========\n" );
    
    for( i = 0 ; i < IOAPIC_MAXIRQTOINTINMAPCOUNT ; i++ )
    {
        Printf(0x17 ,  "IRQ : %d -> INTIN : %d\n", i, gs_stIOAPICManager.IRQToINTINMap[ i ] );
    }
}

/**
 *  IRQ�� ���� APIC ID�� �����ϵ��� ����
 */
void RoutingIRQToAPICID( int iIRQ, BYTE bAPICID )
{
    int i;
    IOREDIRECTIONTABLE stEntry;

    // ���� �˻�
    if( iIRQ > IOAPIC_MAXIRQTOINTINMAPCOUNT )
    {
        return ;
    }
    
    // ������ I/O �����̷��� ���̺��� �о ������(Destination) �ʵ常 ����
    ReadIOAPICRedirectionTable( gs_stIOAPICManager.IRQToINTINMap[ iIRQ ],
            &stEntry );
    stEntry.Destination = bAPICID;
    WriteIOAPICRedirectionTable( gs_stIOAPICManager.IRQToINTINMap[ iIRQ ],
            &stEntry );
}


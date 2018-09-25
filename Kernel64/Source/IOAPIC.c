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
    // I/O APIC를 관리하는 자료구조를 초기화
    //==========================================================================
    MemSet( &gs_stIOAPICManager, 0, sizeof( gs_stIOAPICManager ) );
    
    // I/O APIC의 메모리 맵 I/O 어드레스 저장, 아래 함수에서 내부적으로 처리함
    GetIOAPICBaseAddressOfISA();
    
    // IRQ를 I/O APIC의 INTIN 핀과 연결한 테이블(IRQ->INTIN 매핑 테이블)을 초기화
    for( i = 0 ; i < IOAPIC_MAXIRQTOINTINMAPCOUNT ; i++ )
    {
        gs_stIOAPICManager.IRQToINTINMap[ i ] = 0xFF;
    }
    
    //==========================================================================
    // I/O APIC를 마스크하여 인터럽트가 발생하지 않도록 하고 I/O 리다이렉션 테이블 초기화
    //==========================================================================
    // 먼저 I/O APIC의 인터럽트를 마스크하여 인터럽트가 발생하지 않도록 함
    MaskAllInterruptInIOAPIC();
    
    // IO 인터럽트 지정 엔트리 중에서 ISA 버스와 관련된 인터럽트만 추려서 I/O 리다이렉션
    // 테이블에 설정
    // MP 설정 테이블 헤더의 시작 어드레스와 엔트리의 시작 어드레스를 저장
    pstMPManager = GetMPConfigurationManager();
    pstMPHeader = pstMPManager->MPConfigurationTableHeader;
    qwEntryAddress = pstMPManager->BaseEntryStartAddress;
    
    // 모든 엔트리를 확인하여 ISA 버스와 관련된 I/O 인터럽트 지정 엔트리를 검색
    for( i = 0 ; i < pstMPHeader->EntryCount ; i++ )
    {
        bEntryType = *( BYTE* ) qwEntryAddress;
        switch( bEntryType )
        {
            // IO 인터럽트 지정 엔트리이면, ISA 버스인지 확인하여 I/O 리다이렉션
            // 테이블에 설정하고 IRQ->INITIN 매핑 테이블을 구성
        case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
            pstIOAssignmentEntry = ( IOINTERRUPTASSIGNMENTENTRY* ) qwEntryAddress;

            // 인터럽트 타입이 인터럽트(INT)인 것만 처리
            if( ( pstIOAssignmentEntry->SourceBUSID == pstMPManager->ISABusID ) &&
                ( pstIOAssignmentEntry->InterruptType == MP_INTERRUPTTYPE_INT ) )                        
            {
                // 목적지 필드는 IRQ 0를 제외하고 0x00으로 설정하여 Bootstrap Processor만 전달
                // IRQ 0는 스케줄러에 사용해야 하므로 0xFF로 설정하여 모든 코어로 전달
                if( pstIOAssignmentEntry->SourceBUSIRQ == 0 )
                {
                    bDestination = 0xFF;
                }
                else
                {
                    bDestination = 0x00;
                }
                
                // ISA 버스는 엣지 트리거(Edge Trigger)와 1일 때 활성화(Active High)를
                // 사용
                // 목적지 모드는 물리 모드, 전달 모드는 고정(Fixed)으로 할당
                // 인터럽트 벡터는 PIC 컨트롤러의 벡터와 같이 0x20 + IRQ로 설정
                SetIOAPICRedirectionEntry( &stIORedirectionEntry, bDestination, 
                    0x00, IOAPIC_TRIGGERMODE_EDGE | IOAPIC_POLARITY_ACTIVEHIGH |
                    IOAPIC_DESTINATIONMODE_PHYSICALMODE | IOAPIC_DELIVERYMODE_FIXED, 
                    PIC_IRQSTARTVECTOR + pstIOAssignmentEntry->SourceBUSIRQ );
                
                // ISA 버스에서 전달된 IRQ는 I/O APIC의 INTIN 핀에 있으므로, INTIN 값을
                // 이용하여 처리
                WriteIOAPICRedirectionTable( pstIOAssignmentEntry->DestinationIOAPICINTIN, 
                        &stIORedirectionEntry );
                
                // IRQ와 인터럽트 입력 핀(INTIN)의 관계를 저장(IRQ->INTIN 매핑 테이블 구성)
                gs_stIOAPICManager.IRQToINTINMap[ pstIOAssignmentEntry->SourceBUSIRQ ] =
                    pstIOAssignmentEntry->DestinationIOAPICINTIN;                
            }                    
            qwEntryAddress += sizeof( IOINTERRUPTASSIGNMENTENTRY );
            break;
        
            // 프로세스 엔트리는 무시
        case MP_ENTRYTYPE_PROCESSOR:
            qwEntryAddress += sizeof( PROCESSORENTRY );
            break;
            
            // 버스 엔트리, I/O APIC 엔트리, 로컬 인터럽트 지정 엔트리는 무시
        case MP_ENTRYTYPE_BUS:
        case MP_ENTRYTYPE_IOAPIC:
        case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
            qwEntryAddress += 8;
            break;
        }
    }  
}

/**
 *  IRQ와 I/O APIC의 인터럽트 핀(INT IN)간의 매핑 관계를 출력
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
 *  IRQ를 로컬 APIC ID로 전달하도록 변경
 */
void RoutingIRQToAPICID( int iIRQ, BYTE bAPICID )
{
    int i;
    IOREDIRECTIONTABLE stEntry;

    // 범위 검사
    if( iIRQ > IOAPIC_MAXIRQTOINTINMAPCOUNT )
    {
        return ;
    }
    
    // 설정된 I/O 리다이렉션 테이블을 읽어서 목적지(Destination) 필드만 수정
    ReadIOAPICRedirectionTable( gs_stIOAPICManager.IRQToINTINMap[ iIRQ ],
            &stEntry );
    stEntry.Destination = bAPICID;
    WriteIOAPICRedirectionTable( gs_stIOAPICManager.IRQToINTINMap[ iIRQ ],
            &stEntry );
}


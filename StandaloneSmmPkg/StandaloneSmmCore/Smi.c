/** @file
  SMI management.

  Copyright (c) 2009 - 2013, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available 
  under the terms and conditions of the BSD License which accompanies this 
  distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "StandaloneSmmCore.h"

//
// SMM_HANDLER_REGISTER_NOTIFY
//

#define SMI_REGISTER_NOTIFY_SIGNATURE  SIGNATURE_32('s','m','i','r')

 typedef struct {
  UINTN                                 Signature;
  LIST_ENTRY                            AllEntries;  // All entries

  EFI_SMM_HANDLER_REGISTER_NOTIFY_FN    Notifier;
} SMI_REGISTER_NOTIFY;

//
// SMM_HANDLER_UNREGISTER_NOTIFY
//

#define SMI_UNREGISTER_NOTIFY_SIGNATURE  SIGNATURE_32('s','m','i','u')

 typedef struct {
  UINTN                                   Signature;
  LIST_ENTRY                              AllEntries;  // All entries

  EFI_SMM_HANDLER_UNREGISTER_NOTIFY_FN    Notifier;
} SMI_UNREGISTER_NOTIFY;

LIST_ENTRY mSmiHandlerRegisterNotifierList   = INITIALIZE_LIST_HEAD_VARIABLE(mSmiHandlerRegisterNotifierList);
LIST_ENTRY mSmiHandlerUnregisterNotifierList = INITIALIZE_LIST_HEAD_VARIABLE(mSmiHandlerUnregisterNotifierList);

//
// SMM_HANDLER - used for each SMM handler
//

#define SMI_ENTRY_SIGNATURE  SIGNATURE_32('s','m','i','e')

 typedef struct {
  UINTN       Signature;
  LIST_ENTRY  AllEntries;  // All entries

  EFI_GUID    HandlerType; // Type of interrupt
  LIST_ENTRY  SmiHandlers; // All handlers
} SMI_ENTRY;

#define SMI_HANDLER_SIGNATURE  SIGNATURE_32('s','m','i','h')

 typedef struct {
  UINTN                         Signature;
  LIST_ENTRY                    Link;        // Link on SMI_ENTRY.SmiHandlers
  EFI_SMM_HANDLER_ENTRY_POINT2  Handler;     // The smm handler's entry point
  SMI_ENTRY                     *SmiEntry;
} SMI_HANDLER;

LIST_ENTRY  mRootSmiHandlerList = INITIALIZE_LIST_HEAD_VARIABLE (mRootSmiHandlerList);
LIST_ENTRY  mSmiEntryList       = INITIALIZE_LIST_HEAD_VARIABLE (mSmiEntryList);

/**
  Finds the SMI entry for the requested handler type.

  @param  HandlerType            The type of the interrupt
  @param  Create                 Create a new entry if not found

  @return SMI entry

**/
SMI_ENTRY  *
EFIAPI
SmmCoreFindSmiEntry (
  IN EFI_GUID  *HandlerType,
  IN BOOLEAN   Create
  )
{
  LIST_ENTRY  *Link;
  SMI_ENTRY   *Item;
  SMI_ENTRY   *SmiEntry;

  //
  // Search the SMI entry list for the matching GUID
  //
  SmiEntry = NULL;
  for (Link = mSmiEntryList.ForwardLink;
       Link != &mSmiEntryList;
       Link = Link->ForwardLink) {

    Item = CR (Link, SMI_ENTRY, AllEntries, SMI_ENTRY_SIGNATURE);
    if (CompareGuid (&Item->HandlerType, HandlerType)) {
      //
      // This is the SMI entry
      //
      SmiEntry = Item;
      break;
    }
  }

  //
  // If the protocol entry was not found and Create is TRUE, then
  // allocate a new entry
  //
  if ((SmiEntry == NULL) && Create) {
    SmiEntry = AllocatePool (sizeof(SMI_ENTRY));
    if (SmiEntry != NULL) {
      //
      // Initialize new SMI entry structure
      //
      SmiEntry->Signature = SMI_ENTRY_SIGNATURE;
      CopyGuid ((VOID *)&SmiEntry->HandlerType, HandlerType);
      InitializeListHead (&SmiEntry->SmiHandlers);

      //
      // Add it to SMI entry list
      //
      InsertTailList (&mSmiEntryList, &SmiEntry->AllEntries);
    }
  }
  return SmiEntry;
}

/**
  Manage SMI of a particular type.

  @param  HandlerType    Points to the handler type or NULL for root SMI handlers.
  @param  Context        Points to an optional context buffer.
  @param  CommBuffer     Points to the optional communication buffer.
  @param  CommBufferSize Points to the size of the optional communication buffer.

  @retval EFI_WARN_INTERRUPT_SOURCE_PENDING  Interrupt source was processed successfully but not quiesced.
  @retval EFI_INTERRUPT_PENDING              One or more SMI sources could not be quiesced.
  @retval EFI_NOT_FOUND                      Interrupt source was not handled or quiesced.
  @retval EFI_SUCCESS                        Interrupt source was handled and quiesced.

**/
EFI_STATUS
EFIAPI
SmiManage (
  IN     CONST EFI_GUID  *HandlerType,
  IN     CONST VOID      *Context         OPTIONAL,
  IN OUT VOID            *CommBuffer      OPTIONAL,
  IN OUT UINTN           *CommBufferSize  OPTIONAL
  )
{
  LIST_ENTRY   *Link;
  LIST_ENTRY   *Head;
  SMI_ENTRY    *SmiEntry;
  SMI_HANDLER  *SmiHandler;
  BOOLEAN      SuccessReturn;
  EFI_STATUS   Status;
  
  Status = EFI_NOT_FOUND;
  SuccessReturn = FALSE;
  if (HandlerType == NULL) {
    //
    // Root SMI handler
    //

    Head = &mRootSmiHandlerList;
  } else {
    //
    // Non-root SMI handler
    //
    SmiEntry = SmmCoreFindSmiEntry ((EFI_GUID *) HandlerType, FALSE);
    if (SmiEntry == NULL) {
      //
      // There is no handler registered for this interrupt source
      //
      return Status;
    }

    Head = &SmiEntry->SmiHandlers;
  }

  for (Link = Head->ForwardLink; Link != Head; Link = Link->ForwardLink) {
    SmiHandler = CR (Link, SMI_HANDLER, Link, SMI_HANDLER_SIGNATURE);

    Status = SmiHandler->Handler (
               (EFI_HANDLE) SmiHandler,
               Context,
               CommBuffer,
               CommBufferSize
               );

    switch (Status) {
    case EFI_INTERRUPT_PENDING:
      //
      // If a handler returns EFI_INTERRUPT_PENDING and HandlerType is not NULL then
      // no additional handlers will be processed and EFI_INTERRUPT_PENDING will be returned.
      //
      if (HandlerType != NULL) {
        return EFI_INTERRUPT_PENDING;
      }
      break;

    case EFI_SUCCESS:
      //
      // If at least one of the handlers returns EFI_SUCCESS then the function will return
      // EFI_SUCCESS. If a handler returns EFI_SUCCESS and HandlerType is not NULL then no
      // additional handlers will be processed.
      //
      if (HandlerType != NULL) {
        return EFI_SUCCESS;
      }
      SuccessReturn = TRUE;
      break;

    case EFI_WARN_INTERRUPT_SOURCE_QUIESCED:
      //
      // If at least one of the handlers returns EFI_WARN_INTERRUPT_SOURCE_QUIESCED
      // then the function will return EFI_SUCCESS. 
      //
      SuccessReturn = TRUE;
      break;

    case EFI_WARN_INTERRUPT_SOURCE_PENDING:
      //
      // If all the handlers returned EFI_WARN_INTERRUPT_SOURCE_PENDING
      // then EFI_WARN_INTERRUPT_SOURCE_PENDING will be returned.
      //
      break;

    default:
      //
      // Unexpected status code returned.
      //
      ASSERT (FALSE);
      break;
    }
  }

  if (SuccessReturn) {
    Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  Registers a handler to execute within SMM.

  @param  Handler        Handler service funtion pointer.
  @param  HandlerType    Points to the handler type or NULL for root SMI handlers.
  @param  DispatchHandle On return, contains a unique handle which can be used to later unregister the handler function.

  @retval EFI_SUCCESS           Handler register success.
  @retval EFI_INVALID_PARAMETER Handler or DispatchHandle is NULL.

**/
EFI_STATUS
EFIAPI
SmiHandlerRegister (
  IN  EFI_SMM_HANDLER_ENTRY_POINT2  Handler,
  IN  CONST EFI_GUID                *HandlerType  OPTIONAL,
  OUT EFI_HANDLE                    *DispatchHandle
  )
{
  SMI_HANDLER  *SmiHandler;
  SMI_ENTRY    *SmiEntry;
  LIST_ENTRY   *List;

  if (Handler == NULL || DispatchHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SmiHandler = AllocateZeroPool (sizeof (SMI_HANDLER));
  if (SmiHandler == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SmiHandler->Signature = SMI_HANDLER_SIGNATURE;
  SmiHandler->Handler = Handler;

  if (HandlerType == NULL) {
    //
    // This is root SMI handler
    //
    SmiEntry = NULL;
    List = &mRootSmiHandlerList;
  } else {
    //
    // None root SMI handler
    //
    SmiEntry = SmmCoreFindSmiEntry ((EFI_GUID *) HandlerType, TRUE);
    if (SmiEntry == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    List = &SmiEntry->SmiHandlers;
  }

  SmiHandler->SmiEntry = SmiEntry;
  InsertTailList (List, &SmiHandler->Link);

  *DispatchHandle = (EFI_HANDLE) SmiHandler;

  return EFI_SUCCESS;
}

/**
  Unregister a handler in SMM.

  @param  DispatchHandle  The handle that was specified when the handler was registered.

  @retval EFI_SUCCESS           Handler function was successfully unregistered.
  @retval EFI_INVALID_PARAMETER DispatchHandle does not refer to a valid handle.

**/
EFI_STATUS
EFIAPI
SmiHandlerUnRegister (
  IN EFI_HANDLE  DispatchHandle
  )
{
  SMI_HANDLER  *SmiHandler;
  SMI_ENTRY    *SmiEntry;

  SmiHandler = (SMI_HANDLER *) DispatchHandle;

  if (SmiHandler == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (SmiHandler->Signature != SMI_HANDLER_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  SmiEntry = SmiHandler->SmiEntry;

  RemoveEntryList (&SmiHandler->Link);
  FreePool (SmiHandler);

  if (SmiEntry == NULL) {
    //
    // This is root SMI handler
    //
    return EFI_SUCCESS;
  }

  if (IsListEmpty (&SmiEntry->SmiHandlers)) {
    //
    // No handler registered for this interrupt now, remove the SMI_ENTRY
    //
    RemoveEntryList (&SmiEntry->AllEntries);

    FreePool (SmiEntry);
  }

  return EFI_SUCCESS;
}

/**
  Registers a callback function that this invoked whenever a SMI handler is registered through SmiHandlerRegister()
  If Function == NULL and Registration is an existing registration, then the callback is unhooked.

  @param  Function        Points to the notification function.
  @param  Registration    Successfully returned the registration record
                          that has been added or unhooked.

  @retval EFI_SUCCESS           Handler register success.
  @retval EFI_INVALID_PARAMETER Registration is NULL.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory resource to finish the request.
  @retval EFI_NOT_FOUND         If the registration is not found when Function == NULL.
**/
EFI_STATUS
EFIAPI
SmiHandlerRegisterNotifierRegister (
  IN  EFI_SMM_HANDLER_REGISTER_NOTIFY_FN  Notifier,
  OUT VOID                                **Registration
  )
{
  SMI_REGISTER_NOTIFY *SmiRegisterNotify;
  LIST_ENTRY          *List;

  if (Registration == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Notifier) {
    //
    // If a notifier has been provided then allocate an entry and add it to the
    // list of notifiers.
    //
    SmiRegisterNotify = AllocateZeroPool (sizeof (SMI_REGISTER_NOTIFY));
    if (SmiRegisterNotify == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    SmiRegisterNotify->Signature = SMI_REGISTER_NOTIFY_SIGNATURE;
    SmiRegisterNotify->Notifier = Notifier;

    //
    // Add it to SMI register notifier entry list
    //
    InsertTailList (&mSmiHandlerRegisterNotifierList, &SmiRegisterNotify->AllEntries);  
    *Registration = (VOID *) SmiRegisterNotify;

    return EFI_SUCCESS;
  } else {
    //
    // If a notifier has not been provided then find the entry in the list and
    // free its memory.
    //  
    SmiRegisterNotify = *Registration;
    for (List = mSmiHandlerRegisterNotifierList.ForwardLink;
	 List != &mSmiHandlerRegisterNotifierList;
	 List = List->ForwardLink) {

      if (SmiRegisterNotify == (CR (List, SMI_REGISTER_NOTIFY, AllEntries, SMI_REGISTER_NOTIFY_SIGNATURE))) {
        RemoveEntryList (&SmiRegisterNotify->AllEntries);
	FreePool (SmiRegisterNotify);
	return EFI_SUCCESS;
      }
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Registers a callback function that this invoked whenever a SMI handler is
  unregistered through SmiHandlerUnregister()
  If Function == NULL and Registration is an existing registration, then the
  callback is unhooked.

  @param  Function        Points to the notification function.
  @param  Registration    Successfully returned the registration record
                          that has been added or unhooked.

  @retval EFI_SUCCESS           Handler register success.
  @retval EFI_INVALID_PARAMETER Registration is NULL.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory resource to finish the request.
  @retval EFI_NOT_FOUND         If the registration is not found when Function == NULL.
**/
EFI_STATUS
EFIAPI
SmiHandlerUnregisterNotifierRegister (
  IN  EFI_SMM_HANDLER_UNREGISTER_NOTIFY_FN  Notifier,
  OUT VOID                                  **Registration
  )
{
  SMI_UNREGISTER_NOTIFY *SmiUnregisterNotify;
  LIST_ENTRY          *List;

  if (Registration == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Notifier) {
    //
    // If a notifier has been provided then allocate an entry and add it to the
    // list of notifiers.
    //
    SmiUnregisterNotify = AllocateZeroPool (sizeof (SMI_UNREGISTER_NOTIFY));
    if (SmiUnregisterNotify == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    SmiUnregisterNotify->Signature = SMI_UNREGISTER_NOTIFY_SIGNATURE;
    SmiUnregisterNotify->Notifier = Notifier;

    //
    // Add it to SMI register notifier entry list
    //
    InsertTailList (&mSmiHandlerUnregisterNotifierList, &SmiUnregisterNotify->AllEntries);  
    *Registration = (VOID *) SmiUnregisterNotify;

    return EFI_SUCCESS;
  } else {
    //
    // If a notifier has not been provided then find the entry in the list and
    // free its memory.
    //  
    SmiUnregisterNotify = *Registration;
    for (List = mSmiHandlerUnregisterNotifierList.ForwardLink;
	 List != &mSmiHandlerUnregisterNotifierList;
	 List = List->ForwardLink) {

      if (SmiUnregisterNotify == (CR (List, SMI_UNREGISTER_NOTIFY, AllEntries, SMI_UNREGISTER_NOTIFY_SIGNATURE))) {
        RemoveEntryList (&SmiUnregisterNotify->AllEntries);
	FreePool (SmiUnregisterNotify);
	return EFI_SUCCESS;
      }
    }
  }

  return EFI_NOT_FOUND;
}

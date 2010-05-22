/** @file oyranos_cmm_SANE.c
 *
 *  Oyranos is an open source Colour Management System
 *
 *  @par Copyright:
 *            2009 (C) Yiannis Belias
 *
 *  @brief    Oyranos SANE device backend for Oyranos
 *  @internal
 *  @author   Yiannis Belias <orion@linux.gr>
 *  @par License:
 *            MIT <http://www.opensource.org/licenses/mit-license.php>
 *  @since    2009/07/05
 */

#include <oyranos/oyranos_cmm.h>
#include <sane/sane.h>
#include <lcms.h>

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "SANE_help.c"
/* --- internal definitions --- */

#define WARN( obj_, _fmt, ... )  message( oyMSG_WARN, (oyStruct_s*)obj_, \
                                _DBG_FORMAT_ _fmt, _DBG_ARGS_, __VA_ARGS__ )
#define INFO( obj_, _fmt, ... )  message( oyMSG_DBG, (oyStruct_s*)obj_, \
                                _DBG_FORMAT_ _fmt, _DBG_ARGS_, __VA_ARGS__ )

/* select a own four byte identifier string instead of "dDev" and replace the
 * dDev in the below macros.
 */
#define CMM_NICK "SANE"
#define CMM_BASE_REG OY_TOP_SHARED OY_SLASH OY_DOMAIN_STD OY_SLASH OY_TYPE_STD OY_SLASH "config.device.icc_profile.scanner." CMM_NICK
#define CMM_VERSION {OYRANOS_VERSION_A,OYRANOS_VERSION_B,OYRANOS_VERSION_C}

#define catCMMfunc(nick,func) nick ## func

#define CMMInit                 catCMMfunc( SANE , CMMInit )
#define CMMallocateFunc         catCMMfunc( SANE , CMMallocateFunc )
#define CMMdeallocateFunc       catCMMfunc( SANE , CMMdeallocateFunc )
#define CMMMessageFuncSet       catCMMfunc( SANE , CMMMessageFuncSet )
#define ConfigsFromPatternUsage catCMMfunc( SANE , ConfigsFromPatternUsage )
#define DeviceInfoFromContext_  catCMMfunc( SANE , DeviceInfoFromContext_ )
#define GetDevices              catCMMfunc( SANE , GetDevices )
#define _api8                   catCMMfunc( SANE , _api8 )
#define _rank_map               catCMMfunc( SANE , _rank_map )
#define Configs_FromPattern     catCMMfunc( SANE , Configs_FromPattern )
#define Configs_Modify          catCMMfunc( SANE , Configs_Modify )
#define Config_Rank             catCMMfunc( SANE , Config_Rank )
#define GetText                 catCMMfunc( SANE , GetText )
#define _texts                  catCMMfunc( SANE , _texts )
#define _cmm_module             catCMMfunc( SANE , _cmm_module )
#define _api8_ui                catCMMfunc( oyRE, _api8_ui )
#define Api8UiGetText           catCMMfunc( oyRE, Api8UiGetText )
#define _api8_ui_texts          catCMMfunc( oyRE, _api8_ui_texts )
#define _api8_icon              catCMMfunc( oyRE, _api8_icon )

#define _DBG_FORMAT_ "%s:%d %s()"
#define _DBG_ARGS_ __FILE__,__LINE__,__func__
#define _(x) x
#define STRING_ADD(a,b) sprintf( &a[strlen(a)], b )

const char * GetText                 ( const char        * select,
                                       oyNAME_e            type );
const char * Api8UiGetText           ( const char        * select,
                                       oyNAME_e            type );

oyMessage_f message = 0;

extern oyCMMapi8_s _api8;
oyRankPad _rank_map[];

int ColorInfoFromHandle(const SANE_Handle device_handle, oyOptions_s **options);
int CreateRankMap_(SANE_Handle device_handle, oyRankPad ** rank_map);
int sane_release_handle(oyPointer * handle_ptr);

/* --- implementations --- */

int CMMInit( oyStruct_s * filter )
{
   int error = 0;
   return error;
}

oyPointer CMMallocateFunc(size_t size)
{
   oyPointer p = 0;
   if (size)
      p = malloc(size);
   return p;
}

void CMMdeallocateFunc(oyPointer mem)
{
   if (mem)
      free(mem);
}

/** @func  CMMMessageFuncSet
 *  @brief API requirement
 *
 *  @version Oyranos: 0.1.10
 *  @since   2007/12/12 (Oyranos: 0.1.10)
 *  @date    2009/02/09
 */
int CMMMessageFuncSet(oyMessage_f message_func)
{
   message = message_func;
   return 0;
}

#define OPTIONS_ADD(opts, name) if(!error && name) \
        error = oyOptions_SetFromText( opts, \
                                       CMM_BASE_REG OY_SLASH #name, \
                                       name, OY_CREATE_NEW );

void ConfigsFromPatternUsage(oyStruct_s * options)
{
    /** oyMSG_WARN should make shure our message is visible. */
   WARN( options, "\n %s",
          "The following help text informs about the communication protocol.");
   WARN( options, "%s", help_message);

   return;
}

/** @internal
 * @brief Put all the scanner hardware information in a oyOptions_s object
 *
 * @param[in]  device_context    The SANE_Device to get the options from
 * @param[out] options           The options object to store the H/W info
 *
 * \todo {
 *         * Untested
 *         * Error handling
 *       }
 */
int DeviceInfoFromContext_(const SANE_Device * device_context, oyOptions_s **options)
{
   const char *device_name = device_context->name,
              *manufacturer = device_context->vendor,
              *model = device_context->model,
              *serial = NULL,
              *host = NULL,
              *system_port = NULL;
   int error = 0;

   serial = "unsupported";
   /* if device string starts with net, it is a remote device */
   if (strncmp(device_name,"net:",4) == 0)
      host = "remote";
   else
      host = "localhost";
   /*TODO scsi/usb/parallel port? */
   system_port = "TODO";

   /*TODO make sure the strings get copied, not pointed */
   OPTIONS_ADD(options, device_name)
   OPTIONS_ADD(options, manufacturer)
   OPTIONS_ADD(options, model)
   OPTIONS_ADD(options, serial)
   OPTIONS_ADD(options, system_port)
   OPTIONS_ADD(options, host)

   return error;
}
/** Function GetDevices
 *  @brief Request all devices from SANE
 *
 * @param[out]    device_list       pointer to -> NULL terminated array of SANE_Device's
 * @param[out]    size              the number of devices
 * @return                          0 OK - else error
 *
 */
int GetDevices(const SANE_Device *** device_list, int *size)
{
   int status, s = 0;
   const SANE_Device **dl = NULL;

   INFO( 0, "%s", "Scanning SANE devices..." );
   fflush(NULL);
   status = sane_get_devices(&dl, SANE_FALSE);
   if (status != SANE_STATUS_GOOD) {
      WARN( 0, "%s()\n Cannot get sane devices: %s",
              _sane_strstatus(status));
      fflush(NULL);
      return 1;
   }
   *device_list = dl;

   while (dl[s]) s++;
   *size = s;

   INFO( 0, "OK [%d]", s );
   fflush(NULL);

   return 0;
}

/** Function Configs_FromPattern
 *  @brief   CMM_NICK oyCMMapi8_s scanner devices
 *
 *  @param[in] 	registration	a string to compare ??????
 *  @param[in]		options			read what to do from the options object
 *  @param[out]	s					Return a configuration for each device found
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/01/19 (Oyranos: 0.1.10)
 *  @date    2009/02/09
 */
int Configs_FromPattern(const char *registration, oyOptions_s * options, oyConfigs_s ** s)
{
   oyConfig_s *device = NULL;
   oyConfigs_s *devices = NULL;
   oyOption_s *context_opt = NULL,
              *handle_opt = NULL,
              *version_opt = NULL,
              *name_opt = NULL;
   oyRankPad *dynamic_rank_map = NULL;
   int i, num_devices, g_error = 0, status, call_sane_exit = 0;
   const char *device_name = 0, *command_list = 0, *command_properties = 0;
   const SANE_Device **device_list = NULL;

   message( oyMSG_DBG, (oyStruct_s*)options,
            _DBG_FORMAT_ "Entering %s(). Options:\n%s", _DBG_ARGS_,
            oyOptions_GetText(options, oyNAME_NICK));

   int rank = oyFilterRegistrationMatch(_api8.registration, registration,
                                        oyOBJECT_CMM_API8_S);
   oyAlloc_f allocateFunc = malloc;

   command_list = oyOptions_FindString(options, "command", "list");
   command_properties = oyOptions_FindString(options, "command", "properties");
   device_name = oyOptions_FindString(options, "device_name", 0);

   /* "error handling" section */
   if (rank == 0) {
      message(oyMSG_WARN, (oyStruct_s *) options, _DBG_FORMAT_ "\n "
              "Registration match Failed. Options:\n%s", _DBG_ARGS_,
              oyOptions_GetText(options, oyNAME_NICK));
      return 1;
   }
   if (s == NULL) {
      message(oyMSG_WARN, (oyStruct_s *) options, _DBG_FORMAT_ "\n "
              "oyConfigs_s is NULL! Options:\n%s", _DBG_ARGS_,
              oyOptions_GetText(options, oyNAME_NICK));
      return 1;
   }
   if (*s != NULL) {
      message(oyMSG_WARN, (oyStruct_s *) options, _DBG_FORMAT_ "\n "
              "Devices struct already present! Options:\n%s", _DBG_ARGS_,
              oyOptions_GetText(options, oyNAME_NICK));
      return 1;
   }

#if 0
   if (!device_name && command_properties) {
      message(oyMSG_WARN, (oyStruct_s *) options, _DBG_FORMAT_ "\n "
              "Device_name is mandatory for properties command:\n%s",
              _DBG_ARGS_, oyOptions_GetText(options, oyNAME_NICK));
      return 1;
   }
#endif

   /* "help" call section */
   if (oyOptions_FindString(options, "command", "help") || !options || !oyOptions_Count(options)) {
    /** oyMSG_WARN should make shure our message is visible. */
      ConfigsFromPatternUsage((oyStruct_s *) options);
      return 0;
   }

   context_opt = oyOptions_Find(options, "device_context");
   handle_opt = oyOptions_Find(options, "device_handle");
   name_opt = oyOptions_Find(options, "oyNAME_NAME");

   /*Handle "driver_version" option [IN] */
   check_driver_version(options, &version_opt, &call_sane_exit);

   devices = oyConfigs_New(0);
   if (command_list) {
      /* "list" call section */

      if (device_name &&   /*If a user provides a device_name option,*/
          !context_opt &&  /*and does not need the device_context data,*/
          !name_opt        /*or the oyNAME_NAME description*/
         )
         num_devices = 1;  /*then we can get away without calling GetDevices()*/
      else if (GetDevices(&device_list, &num_devices) != 0) {
         num_devices = 0; /*So that for loop will not run*/
         ++g_error;
      }

      for (i = 0; i < num_devices; ++i) {
         int error = 0;
         const char *sane_name = NULL,
                    *sane_model = NULL;

         if (device_list) {
            sane_name = device_list[i]->name;
            sane_model = device_list[i]->model;
         } else {
            sane_name = device_name;
         }

         /*Handle "device_name" option [IN] */
         if (device_name &&                        /*device_name is provided*/
             sane_name &&                          /*and sane_name has been retrieved*/
             strcmp(device_name, sane_name) != 0)  /*and they don't match,*/
            continue;                              /*then try the next*/

         device = oyConfig_New(CMM_BASE_REG, 0);

         /*Handle "driver_version" option [OUT] */
         if (version_opt) {
            oyOption_s *tmp = oyOption_Copy(version_opt, 0);
            oyOptions_MoveIn(device->backend_core, &tmp, -1);
         }

         /*Handle "device_name" option [OUT] */
         oyOptions_SetFromText(&device->backend_core,
                               CMM_BASE_REG OY_SLASH "device_name",
                               sane_name,
                               OY_CREATE_NEW);

         /*Handle "oyNAME_NAME" option */
         if (name_opt)
            oyOptions_SetFromText(&device->backend_core,
                                  CMM_BASE_REG OY_SLASH "oyNAME_NAME",
                                  sane_model,
                                  OY_CREATE_NEW);

         /*Handle "device_context" option */
         /* SANE Backend protocol states that device_context is *always* returned
          * This is a slight variation: Only when GetDevices() is called will it be returned,
          * unless we call sane_exit*/
         if (device_list && !call_sane_exit) {
            oyBlob_s *context_blob = oyBlob_New(NULL);
            oyOption_s *context_opt = oyOption_New(CMM_BASE_REG OY_SLASH "device_context", 0);

            oyBlob_SetFromData(context_blob, (oyPointer) device_list[i], sizeof(SANE_Device), "sane");
            oyOption_StructMoveIn(context_opt, (oyStruct_s **) & context_blob);
            oyOptions_MoveIn(device->data, &context_opt, -1);
         }

         /*Handle "device_handle" option */
         if (handle_opt && !call_sane_exit) {
            oyCMMptr_s *handle_ptr = NULL;
            SANE_Handle h;
            status = sane_open(sane_name, &h);
            if (status == SANE_STATUS_GOOD) {
               handle_ptr = oyCMMptr_New(allocateFunc);
               oyCMMptr_Set(handle_ptr,
                            "SANE",
                            "handle",
                            (oyPointer)h,
                            "sane_release_handle",
                            sane_release_handle);
               oyOptions_MoveInStruct(&(device->data),
                                      CMM_BASE_REG OY_SLASH "device_handle",
                                      (oyStruct_s **) &handle_ptr, OY_CREATE_NEW);
            } else
              message( oyMSG_WARN, (oyStruct_s*)options,
              _DBG_FORMAT_ "Unable to open sane device \"%s\": %s",
              _DBG_ARGS_,
              sane_name, sane_strstatus(status));
         }

         device->rank_map = oyRankMapCopy(_rank_map, device->oy_->allocateFunc_);

         error = oyConfigs_MoveIn(devices, &device, -1);

         /*Cleanup*/
         if (error) {
            oyConfig_Release(&device);
            ++g_error;
         }
      }

      *s = devices;
   } else if (command_properties) {
      /* "properties" call section */
      int error = 0;
      const SANE_Device *device_context = NULL;
      SANE_Device *aux_context = NULL;
      SANE_Handle device_handle = NULL;

      GetDevices(&device_list, &num_devices);

      if (device_name &&   /*If a user provides a device_name option,*/
          !context_opt &&  /*and does not need the device_context data,*/
          !name_opt        /*or the oyNAME_NAME description*/
         )
         num_devices = 1;  /*then we can get away without calling GetDevices()*/
      else if(!num_devices || !device_list) {
         num_devices = 0; /*So that for loop will not run*/
         ++g_error;
      }

      for (i = 0; i < num_devices; ++i) {
        const char *sane_name = NULL,
                   *sane_model = NULL;

        if (device_list) {
           sane_name = device_list[i]->name;
           sane_model = device_list[i]->model;
        } else {
           sane_name = device_name;
        }

        /*Handle "device_name" option [IN] */
        if (device_name &&                        /*device_name is provided*/
            sane_name &&                          /*and sane_name has been retrieved*/
            strcmp(device_name, sane_name) != 0)  /*and they don't match,*/
           continue;                              /*then try the next*/

        /*Return a full list of scanner H/W &
         * SANE driver S/W color options
         * with the according rank map */

        device = oyConfig_New(CMM_BASE_REG, 0);

        /*Handle "driver_version" option [OUT] */
        if (version_opt) {
           oyOption_s *tmp = oyOption_Copy(version_opt, 0);
           oyOptions_MoveIn(device->backend_core, &tmp, -1);
        }

        /*1a. Get the "device_context"*/
        if (!context_opt) { /*we'll have to get it ourselves*/
              device_context = *device_list;
              while (device_context) {
                 if (strcmp(sane_name,device_context->name) == 0)
                    break;
                 device_context++;
              }
              if (!device_context) {
                message( oyMSG_WARN, (oyStruct_s*)options,
                _DBG_FORMAT_ "device_name does not match any installed device.",
                _DBG_ARGS_);
                 g_error++;
              }
        } else {
           aux_context = (SANE_Device*)oyOption_GetData(context_opt, NULL, allocateFunc);
           device_context = aux_context;
        }

        /*1b. Use the "device_context"*/
        if(device_context)
           error = DeviceInfoFromContext_(device_context, &(device->backend_core));

        /*2a. Get the "device_handle"*/
        if (!handle_opt) {
           status = sane_open( sane_name, &device_handle );
           if (status != SANE_STATUS_GOOD) {
              message( oyMSG_WARN, (oyStruct_s*)options,
              _DBG_FORMAT_ "Unable to open sane device \"%s\": %s",
              _DBG_ARGS_,
              sane_name, sane_strstatus(status));
              g_error++;
           }
        } else {
           device_handle = (SANE_Handle)((oyCMMptr_s*)handle_opt->value->oy_struct)->ptr;
        }

        if (device_handle) {
           /*2b. Use the "device_handle"*/
           ColorInfoFromHandle(device_handle, &(device->backend_core));

           /*3. Create the rank map*/
           error = CreateRankMap_(device_handle, &dynamic_rank_map);
            if (!error)
              device->rank_map = oyRankMapCopy(dynamic_rank_map, device->oy_->allocateFunc_);
        }
        oyConfigs_MoveIn(devices, &device, -1);
      }

      /*Cleanup*/
      free(dynamic_rank_map);
      free(aux_context);

      *s = devices;
   } else {
      /*unsupported, wrong or no command */
      message(oyMSG_WARN, (oyStruct_s *) options, _DBG_FORMAT_ "\n "
              "No supported commands in options:\n%s", _DBG_ARGS_,
              oyOptions_GetText(options, oyNAME_NICK) );
      ConfigsFromPatternUsage((oyStruct_s *) options);
      g_error = 1;
   }

   /*Global Cleanup*/
   if (call_sane_exit) {
     message( oyMSG_DBG, (oyStruct_s*)options,
            _DBG_FORMAT_ "sane_exit()",
            _DBG_ARGS_);
     sane_exit();
   }

   oyOption_Release(&context_opt);
   oyOption_Release(&handle_opt);
   oyOption_Release(&version_opt);
   oyOption_Release(&name_opt );

   message( oyMSG_DBG, (oyStruct_s*)options,
            _DBG_FORMAT_ "Leaving",
            _DBG_ARGS_);
   return g_error;
}

/** Function Configs_Modify
 *  @brief   oyCMMapi8_s SANE scanner manipulation
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/01/19 (Oyranos: 0.1.10)
 *  @date    2009/08/21
 *
 *  \todo { Test }
 */
int Configs_Modify(oyConfigs_s * devices, oyOptions_s * options)
{
   oyOption_s *context_opt = NULL,
              *handle_opt = NULL,
              *version_opt = NULL;
   oyOption_s *version_opt_dev = NULL;
   oyConfig_s *device = NULL;
   int num_devices, g_error = 0;
   int call_sane_exit = 0;
   const char *command_list = NULL,
              *command_properties = NULL;

   oyAlloc_f allocateFunc = malloc;

   INFO( options, "Options:\n%s", oyOptions_GetText(options, oyNAME_NICK));

   /* "error handling" section */
   if (!devices || !oyConfigs_Count(devices)) {
      WARN( options, "No devices given! Options:\n%s",
              oyOptions_GetText(options, oyNAME_NICK) );
      return 1;
   }

   /* "help" call section */
   if (oyOptions_FindString(options, "command", "help") || !options || !oyOptions_Count(options)) {
    /** oyMSG_WARN should make shure our message is visible. */
      ConfigsFromPatternUsage((oyStruct_s *) options);
      return 0;
   }

   num_devices = oyConfigs_Count(devices);
   command_list = oyOptions_FindString(options, "command", "list");
   command_properties = oyOptions_FindString(options, "command", "properties");

   /* Now we get some options [IN], and we already have some devices with
    * possibly already assigned options. Those provided through the input
    * oyOptions_s should take presedence over ::data & ::backend_core ones.
    * OTOH, all device_* options have a 1-1 relationship meaning if
    * one changes, probably all other should. So the simplest [naive] approach
    * would be to ignore all device_* options [IN] that are already in device.
    * Except from driver_version which has a special meaning.
    */

   /* Handle "driver_version" option [IN] */
   /* Check the first device to see if a positive driver_version is provided. */
   /* If not, consult the input options */
   device = oyConfigs_Get(devices, 0);
   version_opt_dev = oyConfig_Find(device, "driver_version");
   if (version_opt_dev && oyOption_GetValueInt(version_opt_dev, 0) > 0)
      call_sane_exit = 0;
   else
      check_driver_version(options, &version_opt, &call_sane_exit);
   oyConfig_Release(&device);
   oyOption_Release(&version_opt_dev);

   if (command_list) {
      /* "list" call section */
      int i;

      for (i = 0; i < num_devices; ++i) {
         const SANE_Device *device_context = NULL;
         SANE_Status status = SANE_STATUS_INVAL;
         oyOption_s *name_opt_dev = NULL,
                    *handle_opt_dev = NULL,
                    *context_opt_dev = NULL;
         const char *sane_name = NULL,
                    *sane_model = NULL;
         int error = 0;

         device = oyConfigs_Get(devices, i);

         INFO( 0, "Backend core:\n%s", oyOptions_GetText(device->backend_core, oyNAME_NICK));
         INFO( 0, "Data:\n%s", oyOptions_GetText(device->data, oyNAME_NICK));

         /*Ignore device without a device_name*/
         if (!oyOptions_FindString(device->backend_core, "device_name", NULL)) {
            WARN( options, "%s",
                  "The \"device_name\" is missing from config object!");
            oyConfig_Release(&device);
            g_error++;
            continue;
         }

         /*Handle "driver_version" option [OUT] */
         version_opt_dev = oyConfig_Find(device, "driver_version");
         if (!version_opt_dev && version_opt) {
            oyOption_s *tmp = oyOption_Copy(version_opt, 0);
            oyOptions_MoveIn(device->backend_core, &tmp, -1);
         }
         oyOption_Release(&version_opt_dev);

         /*Handle "device_context" option */
         /*This is always provided by Configs_FromPattern()
          * [or should be alternatively by the user].
          * Configs_Modify() will not scan for SANE devices
          * because it takes too long*/
         context_opt_dev = oyConfig_Find(device, "device_context");
         if (!context_opt_dev) {
            WARN( options, "%s", "The \"device_context\" option is missing!");
            error = g_error = -1;
         }
         if (!error) {
            device_context = (SANE_Device*)oyOption_GetData(context_opt_dev, NULL, allocateFunc);
            sane_name  = device_context->name;
            sane_model = device_context->model;
         }

         /*Handle "oyNAME_NAME" option */
         name_opt_dev = oyConfig_Find(device, "oyNAME_NAME");
         if (!error && !name_opt_dev && oyOptions_Find(options, "oyNAME_NAME"))
            oyOptions_SetFromText(&device->backend_core,
                                  CMM_BASE_REG OY_SLASH "oyNAME_NAME",
                                  sane_model,
                                  OY_CREATE_NEW);

         /*Handle "device_handle" option */
         handle_opt_dev = oyConfig_Find(device, "device_handle");
         if (!error && !handle_opt_dev) {
            oyCMMptr_s *handle_ptr = NULL;
            SANE_Handle h;
            status = sane_open(sane_name, &h);
            if (status == SANE_STATUS_GOOD) {
               handle_ptr = oyCMMptr_New(allocateFunc);
               oyCMMptr_Set(handle_ptr,
                            "SANE",
                           "handle",
                            (oyPointer)h,
                            "sane_release_handle",
                            sane_release_handle);
               oyOptions_MoveInStruct(&(device->data),
                                      CMM_BASE_REG OY_SLASH "device_handle",
                                      (oyStruct_s **) &handle_ptr, OY_CREATE_NEW);
            } else
               INFO( options, "Unable to open sane device \"%s\": %s",
                     sane_name, sane_strstatus(status) );
         }

         /*Create static rank_map, if not already there*/
         if (!device->rank_map)
            device->rank_map = oyRankMapCopy(_rank_map, device->oy_->allocateFunc_);

         /*Cleanup*/
         oyConfig_Release(&device);
         oyOption_Release(&context_opt_dev);
         oyOption_Release(&name_opt_dev);
         oyOption_Release(&handle_opt_dev);
      }
   } else if (command_properties) {
      /* "properties" call section */
      int i;

      /*Return a full list of scanner H/W &
       * SANE driver S/W color options
       * with the according rank map */

      for (i = 0; i < num_devices; ++i) {
         SANE_Device *device_context = NULL;
         SANE_Status status = SANE_STATUS_INVAL;
         SANE_Handle device_handle;
         oyOption_s *name_opt_dev = NULL,
                    *handle_opt_dev = NULL,
                    *context_opt_dev = NULL;
         oyRankPad *dynamic_rank_map = NULL;
         char *device_name = NULL;

         /* All previous device properties are considered obsolete
          * and a new device is created. Basic options are moved from
          * the old to new device */
         device = oyConfigs_Get(devices, i);

         INFO(0, "Backend core:\n%s", oyOptions_GetText(device->backend_core, oyNAME_NICK));
         INFO(0, "Data:\n%s", oyOptions_GetText(device->data, oyNAME_NICK));

         /*Ignore device without a device_name*/
         if (!oyOptions_FindString(device->backend_core, "device_name", NULL)) {
            WARN( options, "%s",
                 "The \"device_name\" is NULL, or missing from config object!");
            oyConfig_Release(&device);
            g_error++;
            continue;
         }

         /*Handle "driver_version" option [OUT] */
         if (version_opt) {
            oyOption_s *tmp = oyOption_Copy(version_opt, 0);
            oyOptions_MoveIn(device->backend_core, &tmp, -1);
         }

         /* 1. Get the "device_name" from old device */
         name_opt_dev = oyConfig_Find(device, "device_name");
         device_name = oyOption_GetValueText(name_opt_dev, allocateFunc);

         /* 2. Get the "device_context" from old device */
         /* It should be there, see "list" call above */
         context_opt_dev = oyConfig_Find(device, "device_context");
         if (context_opt_dev) {
            device_context = (SANE_Device*)oyOption_GetData(context_opt_dev, NULL, allocateFunc);
            if (device_context) {
               oyOptions_MoveIn(device->data, &context_opt_dev, -1);
            } else {
               WARN( options, "%s", "The \"device_context\" is NULL!");
               oyOption_Release(&context_opt_dev);
               g_error++;
            }
         } else {
           const SANE_Device **device_list = NULL;
           int num = 0;
           if (GetDevices(&device_list, &num) == 0) {
             device_context = *device_list;
             while (device_context) {
               if (strcmp(device_name,device_context->name) == 0)
                  break;
               device_context++;
             }
             if (!device_context) {
               message( oyMSG_WARN, (oyStruct_s*)options,
               _DBG_FORMAT_ "device_name does not match any installed device.",
               _DBG_ARGS_);
               g_error++;
             }
           }
         }

         /* 3. Get the scanner H/W properties from old device */
         if (device_context) {
            DeviceInfoFromContext_(device_context, &(device->backend_core));
         }

         /* 4. Get the "device_handle" from old device */
         /* If not there, get one from SANE */
         handle_opt_dev = oyConfig_Find(device, "device_handle");
         if (handle_opt_dev) {
            device_handle = (SANE_Handle)((oyCMMptr_s*)handle_opt_dev->value->oy_struct)->ptr;
            oyOptions_MoveIn(device->data, &handle_opt_dev, -1);
         } else {
            status = sane_open( device_name, &device_handle );
            if (status != SANE_STATUS_GOOD)
              WARN(0, "Opening sane device \"%s\"[FAIL: %s]",
                       device_name, sane_strstatus(status));
            else
              INFO( options, "Opening sane device \"%s\" [OK]", device_name);
         }

         if (handle_opt_dev || status == SANE_STATUS_GOOD) {
            /* Use the device_handle to get the device color options */
            ColorInfoFromHandle(device_handle, &(device->backend_core));
         }

         /*Cleanup*/
         /* Remove old, add new device */
         oyConfig_Release(&device);

         /*If we had to open a SANE device, we'll have to close it*/
         if (status == SANE_STATUS_GOOD) {
            INFO( options, "sane_close(%s)", device_name);
            sane_close(device_handle);
         }

         free(dynamic_rank_map);
         free(device_name);
      }
   } else if(!oyOptions_FindString(options, "command", "setup") &&
             !oyOptions_FindString(options, "command", "unset")) {
      /*unsupported, wrong or no command */
      message(oyMSG_WARN, (oyStruct_s *) options, _DBG_FORMAT_ "\n "
              "No supported commands in options:\n%s", _DBG_ARGS_,
              oyOptions_GetText(options, oyNAME_NICK) );
      ConfigsFromPatternUsage((oyStruct_s *) options);
      g_error = 0;
   }

   /*Cleanup*/
   if (call_sane_exit) {
      INFO( options, "%s", "sane_exit()" );
      sane_exit();
   }

   oyOption_Release(&version_opt);

   INFO( options, "%s", "Leaving %s");
   return g_error;
}

/** Function Config_Rank
 *  @brief   CMM_NICK oyCMMapi8_s device check
 *
 *  @param[in]     config              the monitor device configuration
 *  @return                            rank value
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/01/26 (Oyranos: 0.1.10)
 *  @date    2009/02/09
 */
int Config_Rank(oyConfig_s * config)
{
   int error = !config, rank = 1;

   if (!config) {
      message(oyMSG_DBG, (oyStruct_s *) config, _DBG_FORMAT_ "\n " "No config argument provided.\n", _DBG_ARGS_);
      return 0;
   }

   if (error <= 0) {
      /* evaluate a driver specific part of the options */
   }

   return rank;
}

const char * Api8UiGetText           ( const char        * select,
                                       oyNAME_e            type )
{
  static char * category = 0;
  if(strcmp(select,"name") == 0 ||
     strcmp(select,"help") == 0)
  {
    /* The "help" and "name" texts are identical, as the module contains only
     * one filter to provide help for. */
    return GetText(select,type);
  }
  else if(strcmp(select, "device_class") == 0)
    {
        if(type == oyNAME_NICK)
            return _("Scanner");
        else if(type == oyNAME_NAME)
            return _("Scanner");
        else
            return _("Scanner data, which come from SANE library.");
    }
  else if(strcmp(select,"category") == 0)
  {
    if(!category)
    {
      /* The following strings must match the categories for a menu entry. */
      const char * i18n[] = {_("Colour"),_("Device"),_("Scanner"),0};
      int len =  strlen(i18n[0]) + strlen(i18n[1]) + strlen(i18n[2]);
      category = (char*)malloc( len + 64 );
      if(category)
        sprintf( category,"%s/%s/%s", i18n[0], i18n[1], i18n[2] );
      else
        message(oyMSG_WARN, (oyStruct_s *) 0, _DBG_FORMAT_ "\n " "Could not allocate enough memory.", _DBG_ARGS_);
    }
         if(type == oyNAME_NICK)
      return "category";
    else if(type == oyNAME_NAME)
      return category;
    else
      return category;
  }
  return 0;
}
const char * _api8_ui_texts[] = {"name", "help", "device_class", "category", 0};

/** @instance _api8_ui
 *  @brief    oydi oyCMMapi4_s::ui implementation
 *
 *  The UI parts for oyRE devices.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/09/06 (Oyranos: 0.1.10)
 *  @date    2009/12/28
 */
oyCMMui_s _api8_ui = {
  oyOBJECT_CMM_DATA_TYPES_S,           /**< oyOBJECT_e       type; */
  0,0,0,                            /* unused oyStruct_s fields; keep to zero */

  CMM_VERSION,                         /**< int32_t version[3] */
  {0,1,10},                            /**< int32_t module_api[3] */

  0, /* oyCMMFilter_ValidateOptions_f */
  0, /* oyWidgetEvent_f */

  "Colour/Device/CameraRaw", /* category */
  0,   /* const char * options */

  0,    /* oyCMMuiGet_f oyCMMuiGet */

  Api8UiGetText,  /* oyCMMGetText_f getText */
  _api8_ui_texts  /* (const char**)texts */
};

oyIcon_s _api8_icon = {
  oyOBJECT_ICON_S, 0,0,0, 0,0,0, (char*)"oyranos_logo.png"
};

/** @instance _rank_map
 *  @brief    oyRankPad map for mapping device to configuration informations
 *
 *  This is the static part for the well known options. The final array will
 *  be created by the oyCreateRankMap_() function.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/01/27 (Oyranos: 0.1.10)
 *  @date    2009/02/09
 */
oyRankPad _rank_map[] = {
   /* Scanner H/W information */
   {"device_name", 2, -1, 0},                     /**< is good */
   {"profile_name", 0, 0, 0},                     /**< non relevant for device properties*/
   {"manufacturer", 1, -1, 0},                    /**< is nice */
   {"model", 5, -5, 0},                           /**< important, should not fail */
   {"serial", 10, 0, 0},                          /**< currently not avaliable */
   {"host", 1, 0, 0},                             /**< currently only local or remote */
   {"system_port", 2, 0, 0},                      /**< good to match */
   {"driver_version", 2, 0, 0},                   /**< good to match */
                                                                      
                                                  /* User supplied information */
   {"media", 1, -1, 0},                           /**< type of paper/film/slide/... */
   {0, 0, 0, 0}                                   /**< end of list */
};

/** @instance _api8
 *  @brief    CMM_NICK oyCMMapi8_s implementations
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/01/19 (Oyranos: 0.1.10)
 *  @date    2009/12/28
 */
oyCMMapi8_s _api8 = {
   oyOBJECT_CMM_API8_S,
   0, 0, 0,
   0,                                                                 /**< next */
   CMMInit,                                                           /**< oyCMMInit_f      oyCMMInit */
   CMMMessageFuncSet,                                                 /**< oyCMMMessageFuncSet_f oyCMMMessageFuncSet */
   CMM_BASE_REG,                                                      /**< registration */
   {0, 1, 0},                                                         /**< int32_t version[3] */
   {0, 1, 10},                                                        /**< int32_t module_api[3] */
   0,                                                                 /**< char * id_ */
   0,                                                                 /**< oyCMMapi5_s * api5_ */
   Configs_FromPattern,                                               /**<oyConfigs_FromPattern_f oyConfigs_FromPattern*/
   Configs_Modify,                                                    /**< oyConfigs_Modify_f oyConfigs_Modify */
   Config_Rank,                                                       /**< oyConfig_Rank_f oyConfig_Rank */
   &_api8_ui,                                                         /**< device class UI name and help */
   &_api8_icon,                                                       /**< device icon */
   _rank_map                                                          /**< oyRankPad ** rank_map */
};

/**
 *  This function implements oyCMMInfoGetText_f.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/12/23 (Oyranos: 0.1.10)
 *  @date    2009/02/09
 *
 *  \todo { Add usage info }
 */
const char *GetText(const char *select, oyNAME_e type)
{
   if (strcmp(select, "name") == 0) {
      if (type == oyNAME_NICK)
         return _(CMM_NICK);
      else if (type == oyNAME_NAME)
         return _("Oyranos Scanner");
      else
         return _("The scanner (hopefully)usefull backend of Oyranos.");
   } else if (strcmp(select, "manufacturer") == 0) {
      if (type == oyNAME_NICK)
         return _("orionas");
      else if (type == oyNAME_NAME)
         return _("Yiannis Belias");
      else
         return
             _
             ("Oyranos project; www: http://www.oyranos.com; support/email: ku.b@gmx.de; sources: http://www.oyranos.com/wiki/index.php?title=Oyranos/Download");
   } else if (strcmp(select, "copyright") == 0) {
      if (type == oyNAME_NICK)
         return _("MIT");
      else if (type == oyNAME_NAME)
         return _("Copyright (c) 2009 Kai-Uwe Behrmann; MIT");
      else
         return _("MIT license: http://www.opensource.org/licenses/mit-license.php");
   } else if (strcmp(select, "help") == 0) {
      if (type == oyNAME_NICK)
         return _("help");
      else if (type == oyNAME_NAME)
         return _("My filter introduction.");
      else
         return _("All the small details for using this module.");
   }
   return 0;
}
const char *_texts[5] = { "name", "copyright", "manufacturer", "help", 0 };

/** @instance _cmm_module
 *  @brief    CMM_NICK module infos
 *
 *  @version Oyranos: 0.1.10
 *  @since   2007/12/12 (Oyranos: 0.1.10)
 *  @date    2009/06/23
 */
oyCMMInfo_s _cmm_module = {

   oyOBJECT_CMM_INFO_S,/**< ::type; the object type */
   0, 0, 0,            /**< static objects omit these fields */
   CMM_NICK,           /**< ::cmm; the four char filter id */
   (char *)"0.2",      /**< ::backend_version */
   GetText,            /**< ::getText; UI texts */
   (char **)_texts,    /**< ::texts; list of arguments to getText */
   OYRANOS_VERSION,    /**< ::oy_compatibility; last supported Oyranos CMM API*/

  /** ::api; The first filter api structure. */
   (oyCMMapi_s *) & _api8,

  /** ::icon; zero terminated list of a icon pyramid */
   {oyOBJECT_ICON_S, 0, 0, 0, 0, 0, 0, "oyranos_logo.png"},
};

/* Helper functions */

/** @internal
 * @brief Put all the SANE backend color information in a oyOptions_s object
 *
 * @param[in]   device_handle          The SANE_Handle to talk to SANE backend
 * @param[out]  options                The options object to store the Color info to
 *
 * \todo { Untested
 *         error handling }
 */
int ColorInfoFromHandle(const SANE_Handle device_handle, oyOptions_s **options)
{
   const SANE_Option_Descriptor *opt = NULL;
   SANE_Int num_options = 0;
   SANE_Status status;
   int error = 0, i;
   unsigned int opt_num = 0, count;
   char cmm_base_reg[] = CMM_BASE_REG OY_SLASH;
   char *value_str = NULL;
   const size_t value_size = 100; /*Better not allow more than 100 characters in the option value string*/

   /* We got a device, find out how many options it has */
   status = sane_control_option(device_handle, 0, SANE_ACTION_GET_VALUE, &num_options, 0);
   if (status != SANE_STATUS_GOOD) {
      message(oyMSG_WARN, 0,
              "%s()\n Unable to determine option count: %s\n",
              __func__, sane_strstatus(status));
      return -1;
   }

   value_str = malloc(sizeof(char)*value_size);

   for (opt_num = 1; opt_num < num_options; opt_num++) {
      opt = sane_get_option_descriptor(device_handle, opt_num);
      if ((opt->cap & SANE_CAP_COLOUR) /*&& !(opt->cap & SANE_CAP_INACTIVE)*/) {
         void *value = malloc(opt->size);
         char *registration = malloc(sizeof(cmm_base_reg)+strlen(opt->name));

         sprintf(registration, "%s%s", cmm_base_reg, opt->name); //FIXME not optimal

         sane_control_option(device_handle, opt_num, SANE_ACTION_GET_VALUE, value, 0);
         switch (opt->type) {
            case SANE_TYPE_BOOL:
               value_str[0] = *(SANE_Bool *) value ? '1' : '0';
               value_str[1] = '\0';
               oyOptions_SetFromText(options, registration, value_str, OY_CREATE_NEW);
               break;
            case SANE_TYPE_INT:
               if (opt->size == (SANE_Int)sizeof(SANE_Word)) {
                  snprintf(value_str, value_size, "%d", *(SANE_Int *) value);
                  oyOptions_SetFromText(options, registration, value_str, OY_CREATE_NEW);
               } else {
                  int count = opt->size/sizeof(SANE_Word);
                if (strstr(opt->name, "gamma-table")) {
                  /* If the option contains a gamma table, calculate the gamma value
                   * as a float and save that instead */
                   LPGAMMATABLE lt = cmsAllocGamma(count);
                   float norm = 65535.0/(count-1);

                   /*Normalise table to 65535. lcms expects that*/
                   for (i=0; i<count; ++i)
                      lt->GammaTable[i] = (WORD)((float)(*(SANE_Int *) value+i)*norm);

                   snprintf(value_str, value_size, "%f", cmsEstimateGamma(lt));
                   oyOptions_SetFromText(options, registration, value_str, OY_CREATE_NEW);
                   cmsFreeGamma(lt);
                } else {
                   int chars = 0;
                   for (i=0; i<count; ++i) {
                     int printed = snprintf(value_str+chars, value_size-chars, "%d, ", *(SANE_Int *) value+i);
                     if (printed >= value_size-chars)
                        break;
                     else
                        chars += printed;
                   }
                   oyOptions_SetFromText(options, registration, value_str, OY_CREATE_NEW);
                }
               }
               break;
            case SANE_TYPE_FIXED:
               if (opt->size == (SANE_Int)sizeof(SANE_Word)) {
                  snprintf(value_str, value_size, "%f", SANE_UNFIX(*(SANE_Fixed *) value));
                  oyOptions_SetFromText(options, registration, value_str, OY_CREATE_NEW);
               } else {
                  int count = opt->size/sizeof(SANE_Word);

                  int chars = 0;
                  for (i=0; i<count; ++i) {
                    int printed = snprintf(value_str+chars,
                                           value_size-chars,
                                           "%f, ",
                                           SANE_UNFIX(*(SANE_Fixed *) value+i));
                    if (printed >= value_size-chars)
                       break;
                    else
                       chars += printed;
                  }
                  oyOptions_SetFromText(options, registration, value_str, OY_CREATE_NEW);
               }
               break;
            case SANE_TYPE_STRING:
               oyOptions_SetFromText(options, registration, (const char *)value, OY_CREATE_NEW);
               break;
            default:
               WARN( opt, "Do not know what to do with option %d", opt->type);
               return 1;
               break;
         }
      free(registration );
      }
   }
   free(value_str);

   return error;
}

/** @internal
 * @brief Create a rank map from a scanner handle
 *
 * @param[in]	device_handle				SANE_Handle
 * @param[out]	rank_map						All scanner options affecting color as a rank map
 *
 * \todo { Untested }
 */
int CreateRankMap_(SANE_Handle device_handle, oyRankPad ** rank_map)
{
   oyRankPad *rm = NULL;

   const SANE_Option_Descriptor *opt = NULL;
   SANE_Int num_options = 0;
   SANE_Status status;

   unsigned int opt_num = 0, i = 0, chars = 0;

   /* Get the nuber of scanner options */
   status = sane_control_option(device_handle, 0, SANE_ACTION_GET_VALUE, &num_options, 0);
   if (status != SANE_STATUS_GOOD) {
      message(oyMSG_WARN, 0,
              "%s()\n Unable to determine option count: %s\n",
              __func__, sane_strstatus(status));
      return -1;
   }

   /* we allocate enough memmory to hold all options */
   rm = calloc(num_options, sizeof(oyRankPad));
   memset(rm, 0, sizeof(oyRankPad) * num_options);

   for (opt_num = 1; opt_num < num_options; opt_num++) {
      opt = sane_get_option_descriptor(device_handle, opt_num);
      if (opt->cap & SANE_CAP_COLOUR) {
         rm[i].key = (char *)malloc(strlen(opt->name) + 1);
         strcpy(rm[i].key, opt->name);
         rm[i].match_value = 5;
         rm[i].none_match_value = -5;
         i++;
      }
   }

   num_options = i + sizeof(_rank_map) / sizeof(oyRankPad); /* color options + static options */
   /* resize rm array to hold only needed options */
   *rank_map = realloc(rm, num_options * sizeof(oyRankPad));
   /* copy static options at end of new rank map */
   memcpy(*rank_map + i, _rank_map, sizeof(_rank_map));

   return 0;
}

/** @internal
 * @brief Release the SANE_Handle.
 *
 * This function is a oyPointer_release_f and is used in the
 * oyCMMptr_s device handle.
 *
 * @param[in]	handle_ptr				SANE_Handle
 * @return 0 for success
 *
 */
int sane_release_handle(oyPointer *handle_ptr)
{
   SANE_Handle h = (SANE_Handle)*handle_ptr;
   sane_close(h);

   printf("SANE handle deleted.\n");
   message(oyMSG_DBG, 0,
           "%s() deleting sane handle: %p\n",
           __func__, h);

   return 0;
}

/** @internal
 * @brief Decide if sane_init/exit will be called
 *
 * 1. Checks for driver_version in options and
 * 2. Calls sane_init if needed
 *
 * @param[in]  options     The input options
 * @param[out] version_opt_p
 * @param[out] call_sane_exit
 * @return 0 for success
 *
 */
int check_driver_version(oyOptions_s *options, oyOption_s **version_opt_p, int *call_sane_exit)
{
   int driver_version = 0, status;
   oyOption_s *context_opt = oyOptions_Find(options, "device_context");
   oyOption_s *handle_opt = oyOptions_Find(options, "device_handle");
   int error = oyOptions_FindInt(options, "driver_version", 0, &driver_version);

   if (!error && driver_version > 0) /*driver_version is provided*/
      *version_opt_p = oyOptions_Find(options, "driver_version");
   else { /*we have to call sane_init()*/
      status = sane_init(&driver_version, NULL);
      if (status == SANE_STATUS_GOOD) {
         INFO( options, "SANE v.(%d.%d.%d) init...OK",
                SANE_VERSION_MAJOR(driver_version),
                SANE_VERSION_MINOR(driver_version),
                SANE_VERSION_BUILD(driver_version));

         if (error &&                     /*we've not been given a driver_version*/
             !context_opt && !handle_opt  /*we've not been given other options*/
            ) {                           /*when we are over*/
            *call_sane_exit = 1;
         } else {
            *version_opt_p = oyOption_New(CMM_BASE_REG OY_SLASH "driver_version", 0); //TODO deallocate
            oyOption_SetFromInt(*version_opt_p, driver_version, 0, 0);
         }
      } else {
        message(oyMSG_WARN, (oyStruct_s *) options, _DBG_FORMAT_ "\n "
                "Unable to init SANE. Giving up.[%s] Options:\n%s", _DBG_ARGS_,
                sane_strstatus(status), oyOptions_GetText(options, oyNAME_NICK));
        return 1;
      }
   }
   return 0;
}

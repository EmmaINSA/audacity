/**********************************************************************

   Audacity: A Digital Audio Editor

   ModuleInterface.h

   Leland Lucius

   Copyright (c) 2014, Audacity Team 
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
   
**********************************************************************/

#ifndef __AUDACITY_MODULEINTERFACE_H__
#define __AUDACITY_MODULEINTERFACE_H__

#include <functional>

#ifdef __WXMAC__
// Needs this for std::function
// Make this go away when Mac moves to a proper C++11 library
#include "../../src/MemoryX.h"
#endif

#include "audacity/Types.h"
#include "audacity/IdentInterface.h"
#include "audacity/PluginInterface.h"

// ============================================================================
//
// Don't even think about adding module types, like effect, importer, etc. in 
// here.  The module interface should not have to change when new types of
// plugins are added to Audacity.
//
// In addition a single module may want to provide multiple plugin types.
// ============================================================================

// ============================================================================
///
/// ModuleInterface is a generic dll or so interface for Audacity, that is
/// used for plug ins.  Classes derived from it can handle more specific plug 
/// in types.
///
// ============================================================================

class ModuleInterface  /* not final */ : public IdentInterface
{
public:
   virtual ~ModuleInterface() {};

   // Called immediately after creation to give the instance a chance to
   // initialize.  Return "true" if initialziation was successful.
   virtual bool Initialize() = 0;

   // Called just prior to deletion to allow releasing any resources.
   virtual void Terminate() = 0;

   // "Paths" returned by FindPluginPaths() and passed back to
   // DiscoverPluginsAtPath() have module-specific meaning.
   // They are not necessarily file system paths to existent files that
   // could be placed in any folder and queried for
   // plugin information.
   // This function returns nonempty only when that is the case, and lists
   // the possible extensions of such files (an empty string in a nonempty
   // array means any file is a candidate).
   virtual wxArrayString FileExtensions() = 0;

   // Returns empty, or else, where to copy a plug-in file or bundle.
   // Drag-and-drop is supported only if FileExtensions() returns nonempty and
   // this function returns nonempty.
   virtual wxString InstallPath() = 0;

   // Modules providing a single or static set of plugins may use
   // AutoRegisterPlugins() to register those plugins.
   virtual bool AutoRegisterPlugins(PluginManagerInterface & pluginManager) = 0;

   // For modules providing an interface to other dynamically loaded plugins,
   // the module returns a list of path names that will be presented to the
   // user as "New" for enablement.
   virtual wxArrayString FindPluginPaths(PluginManagerInterface & pluginManager) = 0;

   // Once the user selects desired paths from FindPluginPaths(),
   // a call to DiscoverPluginsAtPath()
   // will be made to request registration of one or more plugins.  If the module must create
   // an instance of the plugin to register it, then then instance should be deleted
   // after registration.
   // May discover more than one plug-in at the path, and
   // may call-back with paths not equal to path (perhaps appending
   // other information to it).
   // Error message does not need to mention the path and may be nonempty
   // even if some plugins are also discovered successfully.
   // Return value is the number of plugins found.
   using RegistrationCallback =
      std::function<
         const PluginID &(ModuleInterface *, EffectIdentInterface *) >;
   virtual unsigned DiscoverPluginsAtPath(
      const wxString & path, wxString &errMsg,
      const RegistrationCallback &callback =
         PluginManagerInterface::DefaultRegistrationCallback)
         = 0;

   // For modules providing an interface to other dynamically loaded plugins,
   // the module returns true if the plugin is still valid, otherwise false.
   virtual bool IsPluginValid(const wxString & path, bool bFast) = 0;

   // When appropriate, CreateInstance() will be called to instantiate the plugin.
   virtual IdentInterface *CreateInstance(const wxString & path) = 0;

   // When appropriate, DeleteInstance() will be called to delete the plugin.
   virtual void DeleteInstance(IdentInterface *instance) = 0;
};

// ============================================================================
//
// ModuleManagerInterface class
//
// ============================================================================

class ModuleManagerInterface /* not final */
{
public:

   // Modules call this to register their interface
   virtual void RegisterModule(ModuleInterface *module) = 0;
};

// ----------------------------------------------------------------------------
// The default entry point name and the name that will be searched for during
// load if the module has been built as a external library.
// ----------------------------------------------------------------------------
#define MODULE_ENTRY AudacityModule

// ----------------------------------------------------------------------------
// The module entry point prototype
// ----------------------------------------------------------------------------
typedef ModuleInterface *(*ModuleMain)(ModuleManagerInterface *moduleManager,
                                       const wxString *path);

// ----------------------------------------------------------------------------
// If BUILDING_AUDACITY is defined during the current build, it is assumed
// that the module wishes to be embedded in the Audacity executable.
// ----------------------------------------------------------------------------
#if defined(BUILDING_AUDACITY)

// ----------------------------------------------------------------------------
// Since there may be multiple embedded modules, the module entry function will
// be declared static so as not to interfere with other modules during link.
// ----------------------------------------------------------------------------
#define DECLARE_MODULE_ENTRY(name)                    \
static ModuleInterface * name(ModuleManagerInterface *moduleManager, const wxString *path)

// ----------------------------------------------------------------------------
// This will create a class and instnace that will register the module entry
// point during Audacity startup.  At the appropriate time, the entry point
// will be called to create the module instance.
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Provides the base for embedded module registration.  If used, a Register()
// method must be supplied explicitly.
// ----------------------------------------------------------------------------
#define DECLARE_BUILTIN_MODULE_BASE(name)             \
extern void RegisterBuiltinModule(ModuleMain rtn);    \
class name                                            \
{                                                     \
public:                                               \
   name() {Register();}                               \
   void Register();                                   \
};                                                    \
static name name ## _instance;

// ----------------------------------------------------------------------------
// Provides the full embedded module registration process.  Nothing further is
// required (other than supplying the module entry point function).
// ----------------------------------------------------------------------------
#define DECLARE_BUILTIN_MODULE(name)                  \
DECLARE_BUILTIN_MODULE_BASE(name)                     \
void name::Register()                                 \
{                                                     \
   RegisterBuiltinModule(MODULE_ENTRY);               \
}

#else

// ----------------------------------------------------------------------------
// When building as an external module, the entry point must be declared with
// "C" linkage and whatever method is used to make the function externally
// visible.
// ----------------------------------------------------------------------------
#define DECLARE_MODULE_ENTRY(name)                                            \
extern "C" __declspec(dllexport)                                              \
   ModuleInterface * name(ModuleManagerInterface *moduleManager,              \
                          const wxString *path)

// ----------------------------------------------------------------------------
// Define these as empty will effectively remove the embedded registration
// functionality.
// ----------------------------------------------------------------------------
#define DECLARE_BUILTIN_MODULE_BASE(name)
#define DECLARE_BUILTIN_MODULE(name)

#endif

#endif // __AUDACITY_MODULEINTERFACE_H__

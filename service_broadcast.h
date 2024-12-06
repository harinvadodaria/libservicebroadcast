/* MIT License

Copyright (c) 2024, Harin Vadodaria

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */

#ifndef SERVICE_BROADCAST_H_INCLUDED
#define SERVICE_BROADCAST_H_INCLUDED

#include <mysql/components/component_implementation.h> /* REQUIRES_SERVICE_PLACEHOLDER etc */
#include <mysql/components/service_implementation.h> /* DEFINE_BOOL_METHOD etc */
#include <mysql/components/services/dynamic_loader_service_notification.h> /* service notifications */
#include <mysql/components/services/reference_caching.h> /* reference_caching services */
#include <mysql/components/services/registry.h>          /* Registry server */

#include <functional>   /* For std::function */
#include <shared_mutex> /* For std::shared_mutex */

extern REQUIRES_SERVICE_PLACEHOLDER(reference_caching_channel);
extern REQUIRES_SERVICE_PLACEHOLDER(reference_caching_cache);
extern REQUIRES_SERVICE_PLACEHOLDER(reference_caching_channel_ignore_list);
extern REQUIRES_SERVICE_PLACEHOLDER(registry);
extern REQUIRES_SERVICE_PLACEHOLDER(registry_registration);

#define ADD_BROADCAST_SERVICE_PLACEHOLDERS                             \
  REQUIRES_SERVICE_PLACEHOLDER(reference_caching_channel);             \
  REQUIRES_SERVICE_PLACEHOLDER(reference_caching_cache);               \
  REQUIRES_SERVICE_PLACEHOLDER(reference_caching_channel_ignore_list); \
  REQUIRES_SERVICE_PLACEHOLDER(registry_registration);

#define ADD_BROADCAST_SERVICE_DEPENDENCIES                     \
  REQUIRES_SERVICE(reference_caching_channel),                 \
      REQUIRES_SERVICE(reference_caching_cache),               \
      REQUIRES_SERVICE(reference_caching_channel_ignore_list), \
      REQUIRES_SERVICE(registry_registration),

#define ADD_BROADCAST_SERVICE_IMPLEMENTATION(NAME)                          \
  BEGIN_SERVICE_IMPLEMENTATION(NAME,                                        \
                               dynamic_loader_services_loaded_notification) \
  service_broadcast::Dynamic_loader_notification::notify_after_load         \
  END_SERVICE_IMPLEMENTATION();                                             \
  BEGIN_SERVICE_IMPLEMENTATION(NAME,                                        \
                               dynamic_loader_services_unload_notification) \
  service_broadcast::Dynamic_loader_notification::notify_before_unload      \
  END_SERVICE_IMPLEMENTATION();

#define ADD_BROADCAST_SERVICE_PROVIDES(NAME)                           \
  PROVIDES_SERVICE(NAME, dynamic_loader_services_loaded_notification), \
      PROVIDES_SERVICE(NAME, dynamic_loader_services_unload_notification),

namespace service_broadcast {
class Dynamic_loader_notification {
 private:
  static bool notify(const char **services, unsigned int count);
  static const char *service_name_;

 public:
  static void set_service_name(const char *service_name);
  static DEFINE_BOOL_METHOD(notify_after_load,
                            (const char **services, unsigned int count));
  static DEFINE_BOOL_METHOD(notify_before_unload,
                            (const char **services, unsigned int count));
};

using service_callback =
    std::function<bool(const my_h_service *service_handle)>;

bool init(const char *service_name, const char *component_name,
          bool set_default);
bool deinit();

bool broadcast(const service_callback &callback);
}  // namespace service_broadcast
#endif /* SERVICE_BROADCAST_H_INCLUDED */
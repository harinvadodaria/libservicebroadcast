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

#include "service_broadcast.h"

namespace service_broadcast {

class Reference_caching_infra {
 public:
  explicit Reference_caching_infra(const std::string &service_name,
                                   const std::string &exception)
      : service_name_(service_name), exception_(exception) {}
  ~Reference_caching_infra() { (void)deinit(); }
  bool ready() { return ready_; }
  bool init();
  bool deinit();
  bool refresh();
  bool execute(const service_callback &callback);

 private:
  bool ready_{false};
  reference_caching_channel channel_{nullptr};
  reference_caching_cache cache_{nullptr};
  std::string service_name_{};
  std::string exception_{};
  mutable std::shared_mutex lock_;
};

Reference_caching_infra *g_reference_caching_infra = nullptr;

bool init(const char *service_name, const char *component_name,
          bool set_default) {
  if (set_default) {
    std::string implementation = service_name;
    implementation.append(".");
    implementation.append(component_name);
    if (mysql_service_registry_registration->set_default(
            implementation.c_str()))
      return true;
  }

  Dynamic_loader_notification::set_service_name(service_name);
  g_reference_caching_infra =
      new (std::nothrow) Reference_caching_infra(service_name, component_name);
  if (g_reference_caching_infra == nullptr) return true;
  if (g_reference_caching_infra->init()) {
    delete g_reference_caching_infra;
    g_reference_caching_infra = nullptr;
    return true;
  }
  return false;
}

bool deinit() {
  delete g_reference_caching_infra;
  g_reference_caching_infra = nullptr;
  return false;
}

const char *Dynamic_loader_notification::service_name_ = nullptr;

void Dynamic_loader_notification::set_service_name(const char *service_name) {
  service_name_ = service_name;
}

bool Dynamic_loader_notification::notify(const char **services,
                                         unsigned int count) {
  for (unsigned int index = 0; index < count; ++index) {
    auto one_service = services[index];
    const char *dot = strchr(one_service, '.');
    std::string service_name(one_service,
                             static_cast<size_t>(dot - one_service));
    std::string implementation_name(dot + 1);
    if (service_name == service_name_ && g_reference_caching_infra) {
      return g_reference_caching_infra->refresh();
    }
  }
  return false;
}

DEFINE_BOOL_METHOD(Dynamic_loader_notification::notify_after_load,
                   (const char **services [[maybe_unused]],
                    unsigned int count [[maybe_unused]])) {
  try {
    return notify(services, count);
  } catch (...) {
    return true;
  }
}

DEFINE_BOOL_METHOD(Dynamic_loader_notification::notify_before_unload,
                   (const char **services [[maybe_unused]],
                    unsigned int count [[maybe_unused]])) {
  try {
    return notify(services, count);
  } catch (...) {
    return true;
  }
}

bool Reference_caching_infra::init() {
  const char *service_names[] = {service_name_.c_str(), nullptr};
  std::unique_lock write_lock(lock_);
  if (mysql_service_reference_caching_channel->create(service_names,
                                                      &channel_) ||
      mysql_service_reference_caching_channel_ignore_list->add(
          channel_, exception_.c_str()) ||
      mysql_service_reference_caching_cache->create(
          channel_, mysql_service_registry, &cache_)) {
    return true;
  }
  ready_ = true;
  return false;
}

bool Reference_caching_infra::deinit() {
  std::unique_lock write_lock(lock_);
  if (ready_) {
    if (mysql_service_reference_caching_cache->destroy(cache_) ||
        mysql_service_reference_caching_channel->destroy(channel_)) {
      return true;
    }
    ready_ = false;
  }
  return false;
}

bool Reference_caching_infra::refresh() {
  std::unique_lock write_lock(lock_);
  if (!ready_) return true;
  /*
    Reference caching component may remove "password_breach_check"
    from the ignore list. So, we add it back unconditionally.
  */
  (void)mysql_service_reference_caching_channel_ignore_list->add(
      channel_, exception_.c_str());
  if (mysql_service_reference_caching_channel->invalidate(channel_)) {
    return true;
  }
  return false;
}

bool Reference_caching_infra::execute(const service_callback &callback) {
  std::shared_lock read_lock(lock_);
  if (!ready_) return false;
  const my_h_service *refs{nullptr};
  if (!mysql_service_reference_caching_cache->get(cache_, 0, &refs)) {
    for (const my_h_service *one = refs; *one; ++one) {
      if (callback(one)) {
        return true;
      }
    }
  }
  return false;
}

bool broadcast(const service_callback &callback) {
  if (g_reference_caching_infra)
    return g_reference_caching_infra->execute(callback);
  return false;
}
}  // namespace service_broadcast
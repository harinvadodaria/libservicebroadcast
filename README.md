# libservicebroadcast
A static library to provide generic framework for MySQL component service broadcast

ATTENTION: THIS LIBRARY IS CREATED FOR DEMONSTRATION. IT IS NOT PRODUCTION READY CODE. USE AT YOUR OWN RISK.

How does it work:
The library uses combination of reference caching component and service notifications
to provide an infrastructure to broadcast a service call.

How to compile:
1. Obtain MySQL 9.x source code:
   git clone https://github.com/mysql/mysql-server mysql-server
2. Create directory <src>/components/librservicebroadcast and put source code
   for this library in the directory.
3. Compile the server code.

How to use:

Assumption: The component that uses this library implements the service to be broadcasted.

The library provides following functions:

1. bool init(const char *service_name, const char *component_name,
          bool set_default);
   This function should be called from the init method of the component.
   service_name - Name of the service to be broadcasted
   component_name - Component that calls init() function - to avoid recursive calls to component's own implementation of the service.
   set_default - To set component's own implementation of service as default. Recommended.
2. bool deinit();
   This function should be called from the deinit method of the component
3. bool broadcast(const service_callback &callback);
   This function should be used to broadcast a service call to other implementations of the same service.
   It accepts a callback so that my_h_service handle can be typecasted to required service and required
   service can be called.

Example usage: https://github.com/harinvadodaria/password_breach_check


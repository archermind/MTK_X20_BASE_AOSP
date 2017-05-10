#ifndef __BOOT_ROM_LOGIC__
#define __BOOT_ROM_LOGIC__
#include <boost/shared_ptr.hpp>
class boot_rom;

class boot_rom_logic
{
public:
   boot_rom_logic(void);
   ~boot_rom_logic(void);
   static void test(void);
public:
   static status_t security_verify_connection(boost::shared_ptr<boot_rom> brom, string auth, string cert);
};

#endif
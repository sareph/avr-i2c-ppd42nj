#pragma once



#define configRead(opt,ret) _configRead( ret, opt, opt##_ )
#define configUpdate(opt,ret) _configUpdate( ret, opt, opt##_ )
#define configWrite(opt,ret) _configWrite( ret, opt, opt##_ )
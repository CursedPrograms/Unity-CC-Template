#ifndef BACKEND_H
#define BACKEND_H

#define DLL_EXPORT __declspec(dllexport)

extern "C" {
    DLL_EXPORT const char* GetPCSpecs();
}

#endif // BACKEND_H

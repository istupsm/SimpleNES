#include "MainBus.h"
#include <cstring>
#include "Log.h"

namespace sn
{
    MainBus::MainBus() :
        m_RAM(0x800, 0),
        m_cartride(nullptr)
    {
    }

    Byte MainBus::read(Address addr)
    {
        if (addr < 0x2000)
            return m_RAM[addr & 0x7ff];
        else if (addr < 0x4020)
        {
            if (addr < 0x4000) //PPU registers, mirrored
            {
                auto it = m_readCallbacks.find(static_cast<IORegisters>(addr & 0x2007));
                if (it != m_readCallbacks.end())
                    return (it -> second)();
                    //Second object is the pointer to the function object
                    //Dereference the function pointer and call it
                else
                    LOG(Error) << "No read callback registered for I/O register at: " << std::hex << +addr << std::endl;
            }
        }
        else if (addr < 0x6000)
        {
            LOG(Error) << "Expansion ROM read attempted. This is currently unsupported" << std::endl;
        }
        else if (addr < 0x8000)
        {
            if (m_cartride->hasExtendedRAM())
            {
                return m_extRAM[addr - 0x6000];
            }
        }
        else
        {
            if (!one_bank)
                return m_cartride->getROM()[addr - 0x8000];
            else //mirrored
                return m_cartride->getROM()[(addr - 0x8000) & 0x3fff];
        }
        return 0;
    }

    void MainBus::write(Address addr, Byte value)
    {
        if (addr < 0x2000)
            m_RAM[addr & 0x7ff] = value;
        else if (addr < 0x4020)
        {
            if (addr < 0x4000) //PPU registers, mirrored
            {
                auto it = m_writeCallbacks.find(static_cast<IORegisters>(addr & 0x2007));
                if (it != m_writeCallbacks.end())
                    (it -> second)(value);
                    //Second object is the pointer to the function object
                    //Dereference the function pointer and call it
                else
                    LOG(Error) << "No write callback registered for I/O register at: " << std::hex << +addr << std::endl;
            }
        }
        else if (addr < 0x6000)
        {
            LOG(Error) << "Expansion ROM access attempted. This is currently unsupported" << std::endl;
        }
        else if (addr < 0x8000)
        {
            if (m_cartride->hasExtendedRAM())
            {
                m_extRAM[addr - 0x8000] = value;
            }
        }
        else
        {
            LOG(Error) << "ROM memory write attempt\n" << std::endl;
        }
    }

    bool MainBus::loadCartridge(Cartridge* cart)
    {
        m_cartride = cart;
        m_mapper = cart->getMapper();
        auto rom = cart->getROM();
        if (m_mapper != 0)
        {
            LOG(Error) << "Mapper not supported" << std::endl;
            return false;
        }

        if (cart->hasExtendedRAM())
            m_extRAM.resize(0x2000);

        if (rom.size() == 0x4000) //1 bank
        {
            one_bank = true;
        }
        else //2 banks
        {
            one_bank = false;
        }
        return true;
    }

    bool MainBus::setWriteCallback(IORegisters reg, std::function<void(Byte)> callback)
    {
        if (!callback)
        {
            LOG(Error) << "callback argument is nullptr" << std::endl;
            return false;
        }
        return m_writeCallbacks.insert({reg, callback}).second;
    }

    bool MainBus::setReadCallback(IORegisters reg, std::function<Byte(void)> callback)
    {
        if (!callback)
        {
            LOG(Error) << "callback argument is nullptr" << std::endl;
            return false;
        }
        return m_readCallbacks.insert({reg, callback}).second;
    }

};

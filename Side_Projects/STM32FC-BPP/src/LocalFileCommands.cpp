#include "LocalFileCommands.h"

#include <Communication/SerialMessageRouter.h>
#include <STM32EMMC.h>

namespace
{

constexpr uint32_t kEmmcD0 = PC8;
constexpr uint32_t kEmmcD1 = PC9;
constexpr uint32_t kEmmcD2 = PC10;
constexpr uint32_t kEmmcD3 = PC11;
constexpr uint32_t kEmmcCk = PC12;
constexpr uint32_t kEmmcCmd = PD2;

bool emmcReady = false;
bool emmcInitStarted = false;

File openFileOrRoot(const char *path, uint8_t mode = FILE_READ)
{
    if (!path || path[0] == '\0' || strcmp(path, "/") == 0)
    {
        return EMMC.openRoot();
    }

    return EMMC.open(path, mode);
}

bool ensureEmmcReady(Print &out)
{
    if (emmcReady)
    {
        return true;
    }

    if (!emmcInitStarted)
    {
        emmcInitStarted = true;
        out.println("FILE/DBG initializing eMMC");
        out.flush();
    }

    EMMC.setDx(kEmmcD0, kEmmcD1, kEmmcD2, kEmmcD3);
    EMMC.setCK(kEmmcCk);
    EMMC.setCMD(kEmmcCmd);

    emmcReady = EMMC.begin();
    if (emmcReady)
    {
        out.println("FILE/DBG eMMC ready");
        return true;
    }

    if (!emmcReady)
    {
        out.print("FILE/ERR eMMC init failed");
        out.print(" hw=");
        out.print(EMMC.lastError());
        out.print(" fs=");
        out.println(EMMC.lastFsError());
    }

    return emmcReady;
}

bool listRecursive(const char *path, Print &out)
{
    File entry = openFileOrRoot(path);
    if (!entry)
    {
        out.print("FILE/ERR not found: ");
        out.println(path ? path : "/");
        return false;
    }

    if (!entry.isDirectory())
    {
        out.print("FILE ");
        out.print(path);
        out.print('\t');
        out.println(static_cast<unsigned long>(entry.size()));
        entry.close();
        return true;
    }

    bool ok = true;
    while (true)
    {
        File child = entry.openNextFile();
        if (!child)
        {
            break;
        }

        String childPath = child.fullname();
        const bool isDir = child.isDirectory();
        const unsigned long size = isDir ? 0UL : static_cast<unsigned long>(child.size());
        child.close();

        out.print(isDir ? "DIR " : "FILE ");
        out.print(childPath);
        if (!isDir)
        {
            out.print('\t');
            out.println(size);
        }
        else
        {
            out.println();
            if (!listRecursive(childPath.c_str(), out))
            {
                ok = false;
            }
        }
    }

    entry.close();
    return ok;
}

bool copyFileToSerial(const char *path, Print &out)
{
    File file = EMMC.open(path, FILE_READ);
    if (!file)
    {
        out.print("FILE/ERR not found: ");
        out.println(path);
        return false;
    }

    if (file.isDirectory())
    {
        out.print("FILE/ERR not a file: ");
        out.println(path);
        file.close();
        return false;
    }

    out.print("FILE/BOF ");
    out.println(path);

    uint8_t buffer[128];
    while (file.available())
    {
        int bytesRead = file.read(buffer, sizeof(buffer));
        if (bytesRead <= 0)
        {
            break;
        }
        out.write(buffer, static_cast<size_t>(bytesRead));
    }

    file.close();
    out.println();
    out.println("FILE/EOF");
    return true;
}

bool removeRecursive(const char *path, Print &out, size_t &removedCount)
{
    File entry = openFileOrRoot(path);
    if (!entry)
    {
        out.print("FILE/ERR not found: ");
        out.println(path ? path : "/");
        return false;
    }

    bool ok = true;
    if (entry.isDirectory())
    {
        while (true)
        {
            File child = entry.openNextFile();
            if (!child)
            {
                break;
            }

            String childPath = child.fullname();
            child.close();

            if (!removeRecursive(childPath.c_str(), out, removedCount))
            {
                ok = false;
            }
        }

        entry.close();

        if (path && strcmp(path, "/") != 0)
        {
            if (EMMC.rmdir(path))
            {
                removedCount++;
            }
            else
            {
                out.print("FILE/ERR failed to remove dir: ");
                out.println(path);
                ok = false;
            }
        }
    }
    else
    {
        entry.close();

        if (EMMC.remove(path))
        {
            removedCount++;
        }
        else
        {
            out.print("FILE/ERR failed to remove file: ");
            out.println(path);
            ok = false;
        }
    }

    return ok;
}

void handleFileMessage(const char *message, const char *prefix, Stream *source)
{
    (void)prefix;
    Print &out = source ? static_cast<Print &>(*source) : static_cast<Print &>(Serial);

    String input = message ? message : "";
    input.trim();
    if (input.length() == 0)
    {
        stm32fc::printLocalFileCommandHelp(out);
        return;
    }

    const int split = input.indexOf(' ');
    String command = split == -1 ? input : input.substring(0, split);
    String arg = split == -1 ? "" : input.substring(split + 1);
    command.trim();
    arg.trim();
    command.toUpperCase();

    if (command != "LS" && command != "CP" && command != "RM" && command != "CLEAR")
    {
        out.print("FILE/ERR unknown command: ");
        out.println(command);
        stm32fc::printLocalFileCommandHelp(out);
        return;
    }

    if (!ensureEmmcReady(out))
    {
        return;
    }

    if (command == "LS")
    {
        const char *path = arg.length() ? arg.c_str() : "/";
        out.print("FILE/LIST ");
        out.println(path);
        listRecursive(path, out);
        out.println("FILE/OK LS done");
    }
    else if (command == "CP")
    {
        if (arg.length() == 0)
        {
            out.println("FILE/ERR usage: FILE/CP <path>");
            return;
        }

        if (copyFileToSerial(arg.c_str(), out))
        {
            out.println("FILE/OK CP done");
        }
    }
    else if (command == "RM")
    {
        if (arg.length() == 0)
        {
            out.println("FILE/ERR usage: FILE/RM <path>");
            return;
        }

        size_t removedCount = 0;
        if (removeRecursive(arg.c_str(), out, removedCount))
        {
            out.print("FILE/OK RM removed ");
            out.println(static_cast<unsigned long>(removedCount));
        }
    }
    else if (command == "CLEAR")
    {
        const char *path = arg.length() ? arg.c_str() : "/";
        size_t removedCount = 0;
        if (removeRecursive(path, out, removedCount))
        {
            out.print("FILE/OK CLEAR removed ");
            out.println(static_cast<unsigned long>(removedCount));
        }
    }
    else
    {
        out.print("FILE/ERR unknown command: ");
        out.println(command);
        stm32fc::printLocalFileCommandHelp(out);
    }
}

} // namespace

namespace stm32fc
{

void registerLocalFileCommands(astra::SerialMessageRouter &router)
{
    router.withListener("FILE/", handleFileMessage);
}

void printLocalFileCommandHelp(Print &out)
{
    out.println("FILE/OK Commands:");
    out.println("FILE/OK FILE/LS [path]");
    out.println("FILE/OK FILE/CP <path>");
    out.println("FILE/OK FILE/RM <path>");
    out.println("FILE/OK FILE/CLEAR [path]");
    out.println("FILE/OK newline required");
}

} // namespace stm32fc

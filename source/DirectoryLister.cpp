#include "DirectoryLister.h"
#include <cstring>
#include <cstdlib>
#include <string>
#define NO_OF_AMIIBOS_PER_PAGE 20
extern "C"
{
    #include "input.h"
    #include "draw.h"
}

int ShowString(char *str)
{
    u32 key = waitInput();
    return 0;
}

Result DirectoryLister::PopulateEntries(char *directory)
{
    m_readentries = 0;
    u32 entries = 0;
    memset(m_filename, 0, 100);
    strcpy(m_filename, directory);
    Result ret = 0;
    u8 *back_dir = (u8*)"...\0";
    memcpy(&m_entries[0].name[0], &back_dir[0], 4); 
    m_entries[0].attributes |= FS_ATTRIBUTE_DIRECTORY;
    ret = FSUSER_OpenArchive(&m_archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, NULL));
    if(ret) return ret;
    ret = FSUSER_OpenDirectory(&m_fshandle, m_archive, fsMakePath(PATH_ASCII, directory));
    
    if(ret) return ret;

    FS_DirectoryEntry entry;
    uint8_t u8name[0x51];
    for(int i = 1; i < 400; i++)
    {
        ret = FSDIR_Read(m_fshandle, &entries, 1, &entry);
        if(ret || entries == 0) break;
        memset(u8name, 0, 50);
        int len = utf16_to_utf8(u8name, &entry.name[0], 0x51);
        u8name[len] = 0;
        memset(&m_entries[i].name[0], 0, 0x51);
        memcpy(&m_entries[i].name[0], &u8name[0], len);
        m_entries[i].attributes = entry.attributes;
        m_readentries += 1;
    }
    
    m_readentries += 1; // for ...
    ret = FSDIR_Close(m_fshandle);
    if(ret) return ret;
    ret = FSUSER_CloseArchive(m_archive);
    return ret;
}

// Following code has been copied from Rosalina
Result DirectoryLister::ListEntries()
{
    Draw_ClearFramebuffer();
    m_selected = 0;
    int page = 0, prevpage = 0;
    while(true)
    {
        if(page != prevpage)
            Draw_ClearFramebuffer();
        Draw_DrawString(80, 10, COLOR_TITLE, "Wumiibo Menu(Amiibo Selection)");
        Draw_DrawString(5, 230, COLOR_TITLE, "Current Location:");
        Draw_DrawString(110, 230, RGB565(0, 0x3F, 0), GetSelectedFileLocation());
        for(int i = 0; i < NO_OF_AMIIBOS_PER_PAGE && page * NO_OF_AMIIBOS_PER_PAGE + i < m_readentries; i++)
        {
            if((page * NO_OF_AMIIBOS_PER_PAGE + i) == m_selected) 
                Draw_DrawCharacter(5, 30 + 10 * i, COLOR_TITLE, '>');
            else
                Draw_DrawCharacter(5, 30 + 10 * i, COLOR_TITLE, ' ');
            
            u16 color = (m_entries[page * NO_OF_AMIIBOS_PER_PAGE + i].attributes & FS_ATTRIBUTE_DIRECTORY) ? 0x9492 : COLOR_WHITE;
            Draw_DrawString(15, 30 + 10 * i, color, (const char*)m_entries[page * NO_OF_AMIIBOS_PER_PAGE + i].name);
        }
        Draw_FlushFramebuffer();
        u32 key = waitInput();
        if(key & BUTTON_DOWN)
        {
            m_selected++;
        }
        
        if(key & BUTTON_UP)
        {
            m_selected--;
        }

        if(key & BUTTON_A)
        {
            if(m_entries[m_selected].attributes & FS_ATTRIBUTE_DIRECTORY)
            {
                if(m_entries[m_selected].name[0] != 0x2E && m_entries[m_selected].name[1] != 0x2E)
                {
                    ConstructFileLocation();
                    char newname[100];
                    memset(newname, 0, 100);
                    strcpy(newname, m_filename);
                    PopulateEntries(newname);
                    Draw_FlushFramebuffer();
                    Draw_ClearFramebuffer();
                    m_selected = 0;
                }
                else
                {
                    int len = strlen(m_filename);
                    int i = len;
                    for(; i > 0; i--)
                    {
                        if(m_filename[i] == '/')
                            break;
                    }
                    memset(&m_filename[i], 0, len - i);
                    if(m_filename[0] == 0) strcpy(m_filename, "/wumiibo");
                    char newname[100];
                    memset(newname, 0, 100);
                    strcpy(newname, m_filename);
                    PopulateEntries(newname);
                    Draw_FlushFramebuffer();
                    Draw_ClearFramebuffer();
                    m_selected = 0;
                }
                
            }
            else
            {
                break;
            }
        }

        if(key & BUTTON_B)
        {
            m_selected = -1;
            break;
        }
        
        if(key & BUTTON_LEFT)
            m_selected -= NO_OF_AMIIBOS_PER_PAGE;
        
        if(key & BUTTON_RIGHT)
        {
            if(m_selected + NO_OF_AMIIBOS_PER_PAGE < m_readentries)
                m_selected += NO_OF_AMIIBOS_PER_PAGE;
            else if((m_readentries - 1) / NO_OF_AMIIBOS_PER_PAGE == page)
                m_selected %= NO_OF_AMIIBOS_PER_PAGE;
            else
                m_selected = m_readentries - 1;
        }

        if(m_selected < 0)
            m_selected = m_readentries - 1;
        else if(m_selected >= m_readentries)
            m_selected = 0;

        prevpage = page;
        page = m_selected / NO_OF_AMIIBOS_PER_PAGE;

    }
    return m_selected;
}

void DirectoryLister::ConstructFileLocation()
{
    uint8_t *name = m_entries[m_selected].name;
    strcat(m_filename, "/");
    strcat(m_filename, (const char*)name);
}
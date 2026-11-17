#pragma once
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <filesystem>
#include <fstream>
#include <string>
#include "json.hpp"
#include "imgui_stdlib.h"
#include "../src/ForumQr.h"
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"shell32.lib")
using Json = nlohmann::json;
namespace fs = std::filesystem;
static fs::path gameDir, prefPath, leConfigDir, leEntry;
static int mode = 0;
static bool remember = false;
static std::string status;
static Json words = Json::object();
static std::vector<unsigned short> fontRanges;
static std::string Read(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}
static void Save(const fs::path& file,const Json& value) {
    fs::create_directories(file.parent_path());
    auto tmp=file; tmp+=L".tmp."+std::to_wstring(GetCurrentProcessId());
    std::ofstream out(tmp,std::ios::binary);out.exceptions(std::ios::failbit|std::ios::badbit);
    out<<value.dump(2)<<'\n';out.close();
    if(!MoveFileExW(tmp.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("保存失败，请检查目录写入权限。");
}
static void SaveChoices() {
    Save(prefPath,Json{{"remember",remember},{"mode",remember?mode:0},{"le_entry",leEntry.u8string()}});
}
static void SetupFrontend() {
    wchar_t path[MAX_PATH];GetModuleFileNameW(nullptr,path,MAX_PATH);gameDir=fs::path(path).parent_path();
    SHGetFolderPathW(nullptr,CSIDL_LOCAL_APPDATA,nullptr,0,path);prefPath=fs::path(path)/L"GBTGame/FC27/launcher.json";
    GetWindowsDirectoryW(path,MAX_PATH);leConfigDir=fs::path(path).root_path()/L"FC 27 Live Editor";
    try {auto j=Json::parse(Read(prefPath));remember=j.value("remember",false);mode=remember && j.value("mode",0)==1;leEntry=fs::u8path(j.value("le_entry",std::string()));}catch(...){}
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;
    auto font=Read(gameDir/"FC27LEzh_font.b85"),ranges=Read(gameDir/"FC27LEzh_ranges.bin");
    if(!font.empty() && ranges.size()>4 && ranges.size()%2==0){
        fontRanges.resize(ranges.size()/2);memcpy(fontRanges.data(),ranges.data(),ranges.size());
        io.Fonts->AddFontFromMemoryCompressedBase85TTF(font.c_str(),19,nullptr,fontRanges.data());
    } else io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/msyh.ttc",19,nullptr,io.Fonts->GetGlyphRangesChineseFull());
    try{words=Json::parse(Read(gameDir/"loc/eng_us/localize.json")).at("translations");}catch(...){}
    ImGui::StyleColorsDark();auto& s=ImGui::GetStyle();s.WindowPadding={26,22};s.ItemSpacing={12,10};s.FramePadding={12,7};s.FrameRounding=5;s.Colors[ImGuiCol_WindowBg]={.055f,.085f,.105f,1};s.Colors[ImGuiCol_Button]={.07f,.33f,.29f,1};s.Colors[ImGuiCol_ButtonHovered]={.09f,.44f,.37f,1};
}
static void DrawQr() {
    auto origin=ImGui::GetCursorScreenPos();auto* draw=ImGui::GetWindowDrawList();const float scale=4;
    draw->AddRectFilled(origin,{origin.x+kForumQrSize*scale,origin.y+kForumQrSize*scale},0xffffffff);
    for(int y=0;y<kForumQrSize;++y)for(int x=0;x<kForumQrSize;++x)if(kForumQr[y][x]=='1'){
        ImVec2 a(origin.x+x*scale,origin.y+y*scale);draw->AddRectFilled(a,{a.x+scale,a.y+scale},0xff000000);}
    ImGui::Dummy({kForumQrSize*scale,kForumQrSize*scale});
}
static void Launch(HWND hwnd) {
    auto file=mode?leEntry:gameDir/"FC27.exe";
    if(file.empty() || !fs::is_regular_file(file)){status=mode?"请选择已经配置好的 LE 启动入口。":"未找到 FC27.exe，请将启动器放入游戏目录。";return;}
    if(fs::equivalent(file,gameDir/"GBTGame.FC27.exe")){status="启动入口不能选择本程序。";return;}
    auto result=ShellExecuteW(hwnd,L"open",file.c_str(),nullptr,file.parent_path().c_str(),SW_SHOWNORMAL);
    status=(INT_PTR)result>32?"启动请求已发送。":"启动失败，请检查入口。";SaveChoices();
}
static void Home(HWND hwnd) {
    ImGui::TextUnformatted("启动模式");
    if(ImGui::RadioButton("默认游戏",&mode,0))SaveChoices();ImGui::SameLine();
    if(ImGui::RadioButton("LE 模式",&mode,1))SaveChoices();
    if(ImGui::Checkbox("记住启动模式",&remember))SaveChoices();
    if(mode){
        ImGui::TextWrapped("%s",leEntry.empty()?"尚未配置 LE 入口":leEntry.u8string().c_str());
        if(ImGui::Button("选择已有 LE 入口")){
            wchar_t path[MAX_PATH]={};OPENFILENAMEW d={sizeof(d)};d.hwndOwner=hwnd;d.lpstrFilter=L"启动入口\0*.exe;*.lnk\0";d.lpstrFile=path;d.nMaxFile=MAX_PATH;d.Flags=OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR|OFN_NODEREFERENCELINKS;
            if(GetOpenFileNameW(&d)){leEntry=path;SaveChoices();}
        }
    }
    if(ImGui::Button(mode?"启动 LE 模式":"启动游戏",{220,44}))Launch(hwnd);
    ImGui::Spacing();ImGui::Separator();ImGui::Spacing();DrawQr();ImGui::SameLine();ImGui::BeginGroup();
    ImGui::TextUnformatted("汉化作者　A-FEI-");ImGui::TextUnformatted("BBS.GBTGAME.ME");ImGui::TextUnformatted("免费在站点获取");
    if(ImGui::Button("打开论坛"))ShellExecuteW(hwnd,L"open",L"https://bbs.gbtgame.me/",nullptr,nullptr,SW_SHOWNORMAL);ImGui::EndGroup();
}
static void Fields(Json& group) {
    for(auto it=group.begin();it!=group.end();++it){
        auto key=it.key();if(key=="dlls"||key=="game_proc_name"||key=="auto_inject"||key=="dll_inject_delay")continue;
        auto label=words.value(key,key);ImGui::PushID(key.c_str());auto& v=it.value();
        if(v.is_object()){if(ImGui::TreeNode(label.c_str())){Fields(v);ImGui::TreePop();}}
        else if(v.is_array()){if(ImGui::TreeNode(label.c_str())){for(size_t i=0;i<v.size();++i){ImGui::PushID((int)i);
            if(v[i].is_object())Fields(v[i]);else if(v[i].is_number_integer()){int n=v[i];if(ImGui::InputInt("键值",&n))v[i]=n;}else ImGui::TextUnformatted(v[i].dump().c_str());
            ImGui::PopID();}ImGui::TreePop();}}
        else if(v.is_boolean()){bool b=v;if(ImGui::Checkbox(label.c_str(),&b))v=b;}
        else if(v.is_number_integer()){int n=v;if(ImGui::InputInt(label.c_str(),&n))v=n;}
        else if(v.is_number_float()){float n=v;if(ImGui::InputFloat(label.c_str(),&n))v=n;}
        else if(v.is_string()){auto str=v.get<std::string>();if(ImGui::InputText(label.c_str(),&str))v=str;}
        ImGui::PopID();
    }
}
static void Settings(const char* name) {
    struct Config{Json doc;std::string original;};static std::map<std::string,Config> configs;
    auto& c=configs[name];auto file=leConfigDir/name;
    if(!fs::is_regular_file(file)){ImGui::TextUnformatted("未找到此 LE 配置文件。");return;}
    if(c.doc.is_null()){c.original=Read(file);c.doc=Json::parse(c.original);}
    if(ImGui::Button("保存设置")){
        if(Read(file)!=c.original)status="文件已由 LE 更新。请重新读取后保存。";
        else{fs::copy_file(file,file.wstring()+L".gbt.bak",fs::copy_options::overwrite_existing);Save(file,c.doc);c.original=Read(file);status="配置已保存。";}
    }ImGui::SameLine();if(ImGui::Button("重新读取")){c.doc=nullptr;return;}ImGui::Separator();
    ImGui::BeginChild("config",{0,-42});Fields(c.doc);ImGui::EndChild();
}
static ID3D11ShaderResourceView* brandTexture = nullptr;
static void LoadBrand(ID3D11Device* device) {
    auto module = GetModuleHandleW(nullptr);
    auto resource = FindResourceW(module, MAKEINTRESOURCEW(1001), RT_RCDATA);
    if (!resource || SizeofResource(module,resource) != 96*96*4) return;
    auto data = LockResource(LoadResource(module,resource));
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width=desc.Height=96; desc.MipLevels=desc.ArraySize=1;
    desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM; desc.SampleDesc.Count=1;
    desc.Usage=D3D11_USAGE_IMMUTABLE; desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA pixels{data,96*4,0};
    ID3D11Texture2D* texture=nullptr;
    if (SUCCEEDED(device->CreateTexture2D(&desc,&pixels,&texture))) {
        device->CreateShaderResourceView(texture,nullptr,&brandTexture);
        texture->Release();
    }
}
static void DrawFrontend(HWND hwnd) {
    auto size=ImGui::GetIO().DisplaySize;ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize(size);
    ImGui::Begin("GBTGame",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoSavedSettings);
    if (brandTexture) { ImGui::Image(brandTexture,{44,44}); ImGui::SameLine(); }
    ImGui::BeginGroup();ImGui::TextColored({.40f,.90f,.72f,1},"GBTGame");
    ImGui::TextUnformatted("FC 27");ImGui::EndGroup();
    ImGui::SameLine(size.x-66);
    auto corner=ImGui::GetCursorScreenPos();
    if(ImGui::InvisibleButton("关闭启动器",{38,38}))PostMessageW(hwnd,WM_CLOSE,0,0);
    auto* draw=ImGui::GetWindowDrawList();
    if(ImGui::IsItemHovered()){draw->AddRectFilled(corner,{corner.x+38,corner.y+38},IM_COL32(130,49,52,255),5);ImGui::SetTooltip("关闭窗口");}
    draw->AddLine({corner.x+13,corner.y+13},{corner.x+25,corner.y+25},0xffeeeeee,2);
    draw->AddLine({corner.x+25,corner.y+13},{corner.x+13,corner.y+25},0xffeeeeee,2);
    ImGui::Spacing();ImGui::Separator();
    if(ImGui::BeginTabBar("pages")){
        const char* pages[]={"启动","编辑器设置","游戏功能"};for(int i=0;i<3;i++)if(ImGui::BeginTabItem(pages[i])){
            try{if(!i)Home(hwnd);else Settings(i==1?"le_config.json":"le_misc_features.json");}catch(const std::exception& e){status=e.what();}
            ImGui::EndTabItem();}ImGui::EndTabBar();}
    ImGui::TextWrapped("%s",status.c_str());ImGui::End();
}

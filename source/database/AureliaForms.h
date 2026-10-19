#pragma once
#include <functional>
struct AureliaFormField {
    const wchar_t* label;
    std::string value;
    std::vector<std::string> choices;
    bool multiline = false;
};
struct AureliaFormState {
    std::vector<AureliaFormField>* fields;
    std::function<void(const std::vector<std::string>&)> save;
    std::vector<HWND> inputs;
    HWND error = nullptr;
    HFONT font = nullptr;
    bool accepted = false, closed = false;
};
static LRESULT CALLBACK AureliaFormProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto* state = reinterpret_cast<AureliaFormState*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
    if(msg==WM_NCCREATE){state=static_cast<AureliaFormState*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(state));}
    if(!state)return DefWindowProcW(hwnd,msg,wp,lp);
    if(msg==WM_COMMAND && LOWORD(wp)==IDOK){
        try {std::vector<std::string> values;for(HWND field:state->inputs)values.push_back(narrow(controlText(field)));state->save(values);state->accepted=true;DestroyWindow(hwnd);}catch(const std::exception& e){SetWindowTextW(state->error,widen(e.what()).c_str());}return 0;
    }
    if((msg==WM_COMMAND && LOWORD(wp)==IDCANCEL)||msg==WM_CLOSE){DestroyWindow(hwnd);return 0;}
    if(msg==WM_DESTROY){state->closed=true;return 0;}
    return DefWindowProcW(hwnd,msg,wp,lp);
}
static bool runAureliaForm(HWND owner,const wchar_t* title,std::vector<AureliaFormField> fields,
    std::function<void(const std::vector<std::string>&)> save) {
    WNDCLASSW cls{};cls.hInstance=GetModuleHandleW(nullptr);cls.lpfnWndProc=AureliaFormProc;cls.lpszClassName=L"AureliaDataForm";cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);RegisterClassW(&cls);
    const int cols=fields.size()>12?3:(fields.size()>6?2:1);
    const int rows=(static_cast<int>(fields.size())+cols-1)/cols;
    const int width=cols*310+50,height=rows*68+190;
    RECT parent{};GetWindowRect(owner,&parent);AureliaFormState state{&fields,std::move(save)};
    HWND dialog=CreateWindowExW(WS_EX_DLGMODALFRAME,L"AureliaDataForm",title,WS_CAPTION|WS_SYSMENU|WS_POPUP,parent.left+std::max<LONG>(0,(parent.right-parent.left-width)/2),parent.top+24,width,height,owner,nullptr,cls.hInstance,&state);
    if(!dialog)throw std::runtime_error("Could not open the data-entry form.");state.font=createFont(dialog,10,false);
    auto control=[&](DWORD ex,const wchar_t* kind,const wchar_t* text,DWORD style,int x,int y,int w,int h,int id){HWND c=CreateWindowExW(ex,kind,text,WS_CHILD|WS_VISIBLE|style,x,y,w,h,dialog,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),cls.hInstance,nullptr);SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(state.font),TRUE);return c;};
    control(0,L"STATIC",L"AURELIA / CLIENT OPERATIONS",0,20,17,width-60,24,0);
    for(size_t i=0;i<fields.size();++i){int col=static_cast<int>(i)/rows,row=static_cast<int>(i)%rows,x=20+col*310,y=58+row*68;const auto& f=fields[i];control(0,L"STATIC",f.label,0,x,y,290,20,0);HWND field;
        if(f.choices.empty()) {field=control(WS_EX_CLIENTEDGE,L"EDIT",widen(f.value).c_str(),WS_TABSTOP|(f.multiline?(ES_MULTILINE|ES_AUTOVSCROLL|WS_VSCROLL):(ES_AUTOHSCROLL)),x,y+22,285,f.multiline?40:28,300+static_cast<int>(i));SendMessageW(field,EM_SETLIMITTEXT,f.multiline?3000:250,0);}
        else {field=control(0,L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,x,y+22,285,210,300+static_cast<int>(i));int selected=0;bool found=false;for(size_t j=0;j<f.choices.size();++j){auto t=widen(f.choices[j]);SendMessageW(field,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(t.c_str()));if(f.choices[j]==f.value){selected=static_cast<int>(j);found=true;}}if(!found&&!f.value.empty()){auto current=widen(f.value);SendMessageW(field,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(current.c_str()));selected=static_cast<int>(f.choices.size());}SendMessageW(field,CB_SETCURSEL,selected,0);}
        state.inputs.push_back(field);
    }
    state.error=control(0,L"STATIC",L"Amounts are in USD. Required values are validated before saving.",0,20,rows*68+62,width-65,45,0);
    control(0,L"BUTTON",L"Save record",WS_TABSTOP|BS_DEFPUSHBUTTON,width-280,height-82,120,32,IDOK);control(0,L"BUTTON",L"Cancel",WS_TABSTOP,width-150,height-82,110,32,IDCANCEL);
    EnableWindow(owner,FALSE);ShowWindow(dialog,SW_SHOW);if(!state.inputs.empty())SetFocus(state.inputs.front());MSG message{};
    while(!state.closed && GetMessageW(&message,nullptr,0,0)>0){if(!IsDialogMessageW(dialog,&message)){TranslateMessage(&message);DispatchMessageW(&message);}}
    EnableWindow(owner,TRUE);SetForegroundWindow(owner);DeleteObject(state.font);return state.accepted;
}
static double formAmount(const std::string& s){size_t end=0;double n=std::stod(s,&end);if(end!=s.size()||!std::isfinite(n))throw std::runtime_error("Enter a number without currency symbols or commas.");return n;}
static int formInteger(const std::string& s){double n=formAmount(s);if(n!=std::floor(n)||n<0||n>10000000)throw std::runtime_error("Enter a valid whole number.");return static_cast<int>(n);}
static std::string formNumber(double value){std::ostringstream s;s<<std::fixed<<std::setprecision(2)<<value;return s.str();}

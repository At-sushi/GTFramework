
/*========================================================================

	キャラクター管理

	ディレクトリからキャラクターのリストを作成して管理する。

==========================================================================*/

#include "stdafx.h"

#include "define_const.h"//needs GOLUAH_SYSTEM_VERSION
#include "define_macro.h"//needs foreach
#include "global.h"	//needs g_system
#include "charlist.h"


namespace GTF
{

	/*-------------------------------------------------------------------------
		構築
	---------------------------------------------------------------------------*/
	CCharacterList::CCharacterList()
	{
	}


	/*-------------------------------------------------------------------------
		破棄
	---------------------------------------------------------------------------*/
	void CCharacterList::Destroy()
	{
		infolist.clear();
		damelist.clear();
		ringlist.clear();
	}


	/*-------------------------------------------------------------------------

		初期化

		1) "リング" に相当するディレクトリの一覧を生成
		2) 各ディレクトリに関してキャラクターを検索
			2.5) 検索した各キャラクターディレクトリの正当性を検証
		3) キャラクタが1体も見つからなかったリングはリストから削除
		4) Favoriteオプションの不正を正す

	---------------------------------------------------------------------------*/
	void CCharacterList::Initialize()
	{
		Destroy();

		InitializeRingList();// 1)
		if(ringlist.size()==0)return;

		DWORD index = 0;
		foreach(ringlist,CCLRingInfoList,ii){ 
			InitializeRing(index); // 2) , 2.5)
			index++;
		}

		//3)
		CCLRingInfoList::iterator ite = ringlist.begin();
		CCLRingInfoList::iterator itee= ringlist.end();
		for(;ite!=itee;ite++){
			if(ite->ring2serialIndex.size() == 0){
				ringlist.erase(ite);
				ite = ringlist.begin();
				itee= ringlist.end();
			}
		}

		//4)
		UINT i,j;
		for(i=0;i<infolist.size();i++){
			for(j=0;j<infolist[i].fav_opts.size();j++)
			{
				CorrectOption(i,&infolist[i].fav_opts[j].opt);
			}
		}
	}

	//リングディレクトリ一覧の生成
	void CCharacterList::InitializeRingList()
	{
		HANDLE hFind;
		WIN32_FIND_DATA fd;

		hFind = FindFirstFile(".\\char\\*.*", &fd);
		CCL_RINGINFO ringitem;

		if(hFind != INVALID_HANDLE_VALUE) {//ディレクトリが存在する場合
			do {
				if(strcmp(fd.cFileName,".")==0 || strcmp(fd.cFileName,"..")==0);//アレ
				else if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) //ディレクトリﾊｹｰﾝ
				{
					strcpy(ringitem.name,fd.cFileName);
					ringlist.push_back(ringitem);
				}
			} while(FindNextFile(hFind, &fd));
			FindClose(hFind);
		}
	}

	//指定インデックスのリング内を検索
	void CCharacterList::InitializeRing(DWORD index)
	{
		HANDLE hFind;
		WIN32_FIND_DATA fd;
		char ringdir[32];
		char chardir[64];

		sprintf(ringdir,"char\\%s\\*.*",ringlist.at(index).name);
		hFind = FindFirstFile(ringdir, &fd);
		sprintf(ringdir,"char\\%s\\",ringlist.at(index).name);

		if(hFind != INVALID_HANDLE_VALUE) {//ディレクトリが存在する場合
			do {
				if(strcmp(fd.cFileName,".")==0 || strcmp(fd.cFileName,"..")==0);//アレ
				else if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) //ディレクトリﾊｹｰﾝ
				{
					sprintf(chardir,"%s%s",ringdir,fd.cFileName);
					VerifyCharacterDir(chardir,index);//検証
				}
			} while(FindNextFile(hFind, &fd));
			FindClose(hFind);
		}
	}

	//キャラクターディレクトリの検証
	BOOL CCharacterList::VerifyCharacterDir(char *dir,DWORD ringindex)
	{
		char path[256];
		sprintf(path,"%s\\action.dll",dir);
		CDI_CHARACTERINFO info;
		ZeroMemory(&info,sizeof(CDI_CHARACTERINFO));
		info.system_version = GOLUAH_SYSTEM_VERSION;

		CCL_DAMEINFO dameinfo;

		sprintf(dameinfo.name,"");

		HINSTANCE hLib = LoadLibrary(path);
		if(hLib == NULL){		//DLL読み込み失敗
			strcpy(dameinfo.dir,dir);
			dameinfo.dame = CCL_DAME_NODLL;
			damelist.push_back(dameinfo);
			return(FALSE);
		}
		PFUNC_CHARACTERINFO pfunc;
		pfunc = (PFUNC_CHARACTERINFO)GetProcAddress(hLib, "CharacterInfo");
		if(pfunc==NULL){		//関数ポインタ取得失敗
			FreeLibrary(hLib);
			strcpy(dameinfo.dir,dir);
			dameinfo.dame = CCL_DAME_CANTGETFP;
			damelist.push_back(dameinfo);
			return(FALSE);
		}
		if(!(*pfunc)(&info)){	//CharacterInfoがFALSEを返したらダメ
			FreeLibrary(hLib);
			strcpy(dameinfo.dir,dir);
			dameinfo.dame = CCL_DAME_FFAIL;
			damelist.push_back(dameinfo);
			return(FALSE);
		}
		if((info.ver > 800 && info.ver<1000) ||
		   info.ver < 400 /*680*/){		//バージョンのチェック（古いほう）
			strcpy(dameinfo.dir,dir);
			strcpy(dameinfo.name,info.name);
			dameinfo.dame = CCL_DAME_OLDDLL;
			dameinfo.ver = info.ver;
			damelist.push_back(dameinfo);
			FreeLibrary(hLib);
			return(FALSE);
		}
		if(info.ver > CDI_VERSION){//バージョンのチェック（新しいほう）
			strcpy(dameinfo.dir,dir);
			strcpy(dameinfo.name,info.name);
			dameinfo.dame = CCL_DAME_NEWDLL;
			dameinfo.ver = info.ver;
			FreeLibrary(hLib);
			damelist.push_back(dameinfo);
			return(FALSE);
		}
		FreeLibrary(hLib);
		//構造体へデータコピー
		CCL_CHARACTERINFO newitem;
		ZeroMemory(&newitem,sizeof(CCL_CHARACTERINFO));
		strcpy(newitem.dir  , dir);
		strcpy(newitem.name , info.name);
		newitem.ver = info.ver;
		newitem.caps = info.caps;

		//オプション項目をコピーする
		CHARACTER_LOAD_OPTION opt;
		for(DWORD i=0;i<32;i++){
			if(strlen(info.options[i].name)!=0){
				memcpy(&opt,&info.options[i],sizeof(CHARACTER_LOAD_OPTION));
				newitem.options.push_back(opt);
			}
		}
		newitem.maxpoint = info.max_option_point;

		//favoriteopt.txt を読み込み
		LoadFavoriteOptions( dir , newitem.fav_opts );

		//リストに追加する
		ringlist.at(ringindex).ring2serialIndex.push_back(infolist.size());//指定リングにインデックス追加
		infolist.push_back(newitem);//一覧に追加

		return(YEAH);
	}

	//********************************************************************
	// Get～系
	//********************************************************************

	int CCharacterList::GetCharacterCount()
	{
		return infolist.size();
	}

	int CCharacterList::GetCharacterCountRing(UINT ring)
	{
		if(ringlist.size()-1 < ring){
			g_system.LogWarning("%s index不正(%d/%d)",__FUNCTION__,ring,ringlist.size());
			return 0;
		}

		return ringlist.at(ring).ring2serialIndex.size();
	}

	char* CCharacterList::GetCharacterDir(UINT index)
	{
		static char *error_return = "error";
		if(index>=infolist.size()){
			g_system.LogWarning("%s index不正(%d/%d)",__FUNCTION__,index,infolist.size());
			return error_return;
		}

		return  infolist.at(index).dir;
	}


	char* CCharacterList::GetCharacterDir(UINT index,int ring)
	{
		if(ring<0)return GetCharacterDir(index);
		return  GetCharacterDir( RingIndex2SerialIndex(ring,index) );
	}

	char* CCharacterList::GetCharacterName(UINT index)
	{
		static char *error_return = "error";
		if(index>=infolist.size()){
			g_system.LogWarning("%s index不正(%d/%d)",__FUNCTION__,index,infolist.size());
			return error_return;
		}

		return  infolist.at(index).name;
	}

	DWORD CCharacterList::GetCharacterVer(UINT index)
	{
		if(index>=infolist.size()){
			g_system.LogWarning("%s index不正(%d/%d)",__FUNCTION__,index,infolist.size());
			return 0;
		}
		return  infolist.at(index).ver;
	}

	int CCharacterList::GetDameCharCount()
	{
		return  damelist.size();
	}

	char* CCharacterList::GetDameCharDir(UINT index)
	{
		return damelist.at(index).dir;
	}
	char* CCharacterList::GetDameCharName(UINT index)
	{
		return(damelist[index].name);
	}
	DWORD CCharacterList::GetDameCharReas(UINT index)
	{
		return(damelist[index].dame);
	}
	DWORD CCharacterList::GetDameCharVer(UINT index)
	{
		return(damelist[index].ver);
	}
	DWORD CCharacterList::RingIndex2SerialIndex(UINT ring,UINT index)
	{
		if(ring > ringlist.size()-1){
			g_system.LogWarning("%s ring不正(%d/%d)",__FUNCTION__,ring,ringlist.size());
			return 0;
		}
		if(ringlist.at(ring).ring2serialIndex.size()<=index){
			g_system.LogWarning("%s index不正(%d/%d)",__FUNCTION__,index,infolist.size());
			return 0;
		}

		return  ringlist.at(ring).ring2serialIndex.at(index)  ;
	}

	int CCharacterList::FindCharacter(char *name)
	{
		if(name==NULL)return(-1);

		for(DWORD i=0;i<infolist.size();i++){
			if(strcmp(infolist[i].name,name)==0)return(i);
		}
		return(-1);
	}



	/*～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～

							オプション系？

	  ～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～*/
	CCOptionSelecter* CCharacterList::GetOptionSelecter(DWORD cindex)
	{
		if(cindex>infolist.size())return(NULL);
		return new CCOptionSelecter( 
			&infolist[cindex],
			infolist[cindex].maxpoint);
	}


	/*～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～

							オプション系？

	  ～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～～*/
	CCSimpleOptionSelecter* CCharacterList::GetSimpleOptionSelecter(DWORD cindex)
	{
		if(cindex>infolist.size())return(NULL);
		return new CCSimpleOptionSelecter( 
			&infolist[cindex],
			infolist[cindex].maxpoint);
	}

	DWORD CCharacterList::GetRandomOption(DWORD index)
	{
		if(index >= infolist.size())return 0;
		if( infolist[index].options.size()==0 )return 0;

		DWORD point_now = infolist[index].maxpoint;
		DWORD ret=0;

		std::list<CHARACTER_LOAD_OPTION> dlist;
		std::list<CHARACTER_LOAD_OPTION>::iterator i;
		CharOptionList::iterator i2;
		for(i2=infolist[index].options.begin();i2!=infolist[index].options.end();i2++){
			dlist.push_back(*i2);
		}

		DWORD breaker=0;
		while( point_now!=0 && dlist.size()!=0){
			for(i=dlist.begin();i!=dlist.end();i++){
				if(i->point > (int)point_now){//ポイントチェック
					dlist.erase(i);
					break;
				}
				else if(ret & i->exclusive){//競合チェック
					dlist.erase(i);
					break;
				}
				else if(rand()%4==0){
					if( (i->depends&ret)==i->depends ){//依存チェック
						point_now -= i->point;
						ret |= i->flag;
						dlist.erase(i);
						break;
					}
				}
			}
			if(breaker++>1000)break;
		}
		return ret;
	}


	char* CCharacterList::GetRingName(UINT index)
	{
		if( ringlist.size()-1 < index )return NULL;
		return ringlist.at(index).name;
	}

	int CCharacterList::GetRingNum()
	{
		return ringlist.size();
	}

	//不正なオプションを除外する
	void CCharacterList::CorrectOption(UINT cindex,DWORD *opt_org)
	{
		if(cindex >= infolist.size()){
			*opt_org = 0;
			return;
		}

		DWORD ret = *opt_org;
		DWORD crnt_flag;
		DWORD crnt_depend;
		DWORD crnt_exclusive;

		BOOL changed=TRUE;
		BOOL fail;
		CharOptionList::iterator i,ie;
		CharOptionList* optlist = &infolist[cindex].options;

		while(changed)
		{
			changed = FALSE;
			crnt_flag =0x00000001;
			for(int b=0;b<32;b++){
				if(ret&crnt_flag)
				{
					fail=FALSE;
					//存在チェック
					{
						fail = TRUE;
						i =optlist->begin();
						ie=optlist->end();
						for(;i!=ie;i++){
							if( i->flag & crnt_flag ){//== crnt_flag ){
								fail=FALSE;
								crnt_exclusive = i->exclusive;
								crnt_depend = i->depends;
								break;
							}
						}
					}
					//依存チェック
					if(!fail){
						if((ret&crnt_depend) != crnt_depend){
							fail=TRUE;
						}
					}
					//排他チェック
					if(!fail){
						if(ret&crnt_exclusive){
							fail=TRUE;
						}
					}

					if(fail){
						ret &= ~crnt_flag;
						changed = TRUE;
					}
				}
				crnt_flag <<= 1;//1ビットシフト
			}
		}

		*opt_org = ret;
	}


	/*----------------------------------------------------------------
		optset.txt の読み込み
	------------------------------------------------------------------*/
	void CCharacterList::LoadFavoriteOptions(char* dir,FavoriteOptionList& list)
	{
		char *path = new char[MAX_PATH];
		sprintf( path, "%s\\optset.txt",dir);

		AkiFile file;
		if(! file.Load( path ) ){
			delete [] path;
			return;
		}

		char *optstr= new char [128];

		FAVORITE_OPTION opt;
		DWORD i=0,j=0;
		int ssret;
		std::vector<DWORD> optlist;
		while(i<file.m_size-2)
		{
			if(file.m_buffer[i] == '#')
			{
				ssret = sscanf((const char*)(&file.m_buffer[i+1]),"%s %s",opt.name,optstr);
				if(ssret==EOF)break;
				else if(ssret==2 && strlen(optstr)==32)
				{
					opt.opt = 0;
					for(int b=0;b<32;b++){
						if(optstr[b]=='1'){
							opt.opt |= 0x00000001;
						}
						else if(optstr[b]!='0'){
							gbl.ods("2進数オプション指定読み込みエラー(%s)",optstr);
							break;
						}
						if(b!=31)opt.opt <<= 1;
					}
					if(opt.opt!=0)
					{
						list.push_back(opt);
					}
				}
			}
			i++;
		}

		delete [] optstr;
		delete [] path;
	}


	/*****************************************************************
		キャラ選択オプションセレクター
	******************************************************************/

	CCOptionSelecter::CCOptionSelecter(CCL_CHARACTERINFO *cinfo,DWORD maxp)
	{
		m_ref_cinfo = cinfo;
		CharOptionList* col = &(cinfo->options);
		m_current_favorite = cinfo->previous_favorite;

		DWORD ini_opt = m_current_favorite==0 ? cinfo->previous_option : cinfo->fav_opts[m_current_favorite-1].opt;

		list = col;
		maxpoint = maxp;
		state = 0;
		counter = 0;
		current_point = maxp;
		current_selected = 0;
		for(int i=0;i<32;i++){
			enabled[i] = FALSE;
		}
		commit_enabled = TRUE;

		offset_x = 0;
		z = -0.2f;

		Initialize(ini_opt);
		m_current_favorite = cinfo->previous_favorite;
	}

	//指定値を初期値として設定する
	void CCOptionSelecter::Initialize(DWORD ini_opt)
	{
		current_point = maxpoint;
		for(int i=0;i<32;i++){
			enabled[i] = FALSE;
		}

		//初期値設定
		DWORD k=0;
		for(CharOptionList::iterator ite=list->begin();ite!=list->end();ite++,k++){
			if(ite->flag & ini_opt)
			{
				enabled[k] = TRUE;
				current_point -= ite->point;

				//gbl.ods("... %s %d %d %X",ite->name,ite->point,current_point,ite->flag & ini_opt);
			}
		}
		m_current_favorite = 0;//free
	}

	//パッド入力を処理する
	BOOL CCOptionSelecter::HandlePad(DWORD inputIndex)
	{
		counter++;
		CharOptionList::iterator list2=list->begin();

		DWORD input = g_input.GetKey(inputIndex,0);

		//↑↓入力処理
		if(input & KEYSTA_UP2){
			current_selected--;
			if(current_selected>list->size())current_selected = list->size();
		}
		else if(input & KEYSTA_DOWN2){
			current_selected++;
			if(current_selected>list->size())current_selected = 0;
		}

		DWORD curr_flag = GetSettings();

		//←・→すこし押しっぱで反応するようにする
		DWORD key1 = g_input.GetKey(inputIndex,1);
		DWORD key2 = g_input.GetKey(inputIndex,2);
		DWORD key3 = g_input.GetKey(inputIndex,3);
		BOOL leftON  = (input&KEYSTA_ALEFT) && (key1&KEYSTA_ALEFT) && (key2&KEYSTA_ALEFT) && (key3&KEYSTA_ALEFT2);
		BOOL rightON = (input&KEYSTA_ARIGHT) && (key1&KEYSTA_ARIGHT) && (key2&KEYSTA_ARIGHT) && (key3&KEYSTA_ARIGHT2);

		//B入力処理(optset変更)
		if(input & KEYSTA_BB2)
		{
			m_current_favorite++;
			if(m_current_favorite == m_ref_cinfo->fav_opts.size()+1){
				m_current_favorite = 0;
			}
			else
			{
				DWORD current_favorite = m_current_favorite;
				Initialize( m_ref_cinfo->fav_opts[m_current_favorite-1].opt );
				m_current_favorite = current_favorite;
			}
			//point over check
			if(current_point<0)
				commit_enabled=FALSE;
			else
				commit_enabled=TRUE;

			//Free以外の場合はＯＫへカーソル移動
			if(m_current_favorite!=0){
				current_selected = list->size();
			}
		}
		//optset設定から上下キーが入ったらFree
		if(m_current_favorite!=0){
			if(current_selected != list->size())
			{
				m_current_favorite=0;
			}
		}

		//C入力処理(ランダム)
		if( input&(KEYSTA_BC2) )
		{
			SetRandom();
			//point over check
			if(current_point<0)
				commit_enabled=FALSE;
			else
				commit_enabled=TRUE;
			current_selected = list->size();//OK位置にカーソル移動
		}

		// ON/OFF
		if(input&(KEYSTA_BA2) || leftON || rightON)
		{
			if(current_selected==list->size())
			{
				if((commit_enabled||g_config.IsLimiterCut()) && input&(KEYSTA_BA2)){//A入力処理(決定)
					state=1;
					counter=0;
					return TRUE;
				}
			}
			//turn off
			else if(enabled[current_selected]){
				current_point += list2[current_selected].point;
				enabled[current_selected] = FALSE;
				TurnOffDependFlags(list2[current_selected].flag);
			}
			//turn on
			else{
				if((list2[current_selected].depends & curr_flag)==list2[current_selected].depends){
					current_point -= list2[current_selected].point;
					enabled[current_selected] = TRUE;
					TurnOffExclisiveFlags(list2[current_selected].exclusive);
				}
			}
			//point over check
			if(current_point<0)
				commit_enabled=FALSE;
			else
				commit_enabled=TRUE;
		}

		if( input&(KEYSTA_BD2) )
		{
			this->state=0xFFFFFFFF;
			return TRUE;
		}
		return FALSE;
	}

	//flagに依存するフラグを全てOFF
	void CCOptionSelecter::TurnOffDependFlags( DWORD flag )
	{
		DWORD next_flag=0;
		DWORD k=0;
		for(CharOptionList::iterator ite=list->begin();ite!=list->end();ite++,k++){
			if(ite->depends & flag){
				if(enabled[k] ){
					enabled[k] = FALSE;
					current_point += ite->point;
					next_flag |= ite->flag;
				}
			}
		}
		if(next_flag)TurnOffDependFlags( next_flag );
	}

	//flagと競合するフラグを全てOFF
	void CCOptionSelecter::TurnOffExclisiveFlags( DWORD ex_flag )
	{
		DWORD k=0;
		DWORD next_flag=0;
		for(CharOptionList::iterator ite=list->begin();ite!=list->end();ite++,k++){
			if(ite->flag & ex_flag){
				if(enabled[k] ){
					enabled[k] = FALSE;
					current_point += ite->point;
					next_flag |= ite->flag;
				}
			}
		}
		if(next_flag)TurnOffDependFlags( next_flag );
	}

	//描画する。StartDrawがかかってる状態で呼び出すこと
	void CCOptionSelecter::Draw()
	{
		int y = 240-list->size()*16;
		int off_x2 = 20;

		char *tmp_str;
		tmp_str = new char[128];

		DWORD ex_flag =0;
		DWORD k=0;
		CharOptionList::iterator ite;
		for(ite=list->begin();ite!=list->end();ite++){
			if(enabled[k]){
				ex_flag |= ite->exclusive;
			}
			k++;
		}

		sprintf(tmp_str,"=--OPTIONS-- POINT:%d",current_point);
		g_system.DrawBMPText(offset_x,y,z,tmp_str,0xFFBBBBBB);

		y+=32;

		k=0;
		DWORD color;
		DWORD setting_now = GetSettings();
		BOOL not_available;
		for(ite=list->begin();ite!=list->end();ite++){
			not_available = FALSE;
			//色は何色？
			if(ite->flag & ex_flag){//競合フラグアリ
				if(k!=current_selected)color=0xFFBBBB00;
				else color=0xFFFFFF00;
				not_available = TRUE;
			}
			else if(enabled[k]){//有効状態
				if(k!=current_selected)color=0xFF0000BB;
				else color=0xFF0000FF;
			}
			else {//無効状態
				if((ite->depends&setting_now)!=ite->depends){//dependsフラグが足りない
					if(k!=current_selected)color=0xFFBBBB00;
					else color=0xFFFFFF00;
					not_available = TRUE;
				}
				else if(current_point<0 || current_point < ite->point){//ONにするとポイント足りない
					if(k!=current_selected)color=0xFFBB0000;
					else color=0xFFFF0000;
				}
				else{//ONにしてもポイントは足りる
					if(k!=current_selected)color=0xFFBBBBBB;
					else color=0xFFFFFFFF;
				}
			}
			//描画
			g_system.DrawBMPText(offset_x+off_x2,y,z,ite->name,color);
			if(not_available)
				g_system.DrawBMPText(offset_x+320,y,z,"NA",color);
			else if(enabled[k])
				g_system.DrawBMPText(offset_x+320,y,z,"ON",color);
			else
				g_system.DrawBMPText(offset_x+320,y,z,"OFF",color);
			y+=32;
			k++;
		}

		//--OK--
		if(commit_enabled){
			if(current_selected!=list->size())
				g_system.DrawBMPText(offset_x+150,y,z,"--OK--",0xFFBBBBBB);
			else
				g_system.DrawBMPText(offset_x+150,y,z,"--OK--",0xFFFFFFFF);
		}
		else{
			if(current_selected!=list->size())
				g_system.DrawBMPText(offset_x+150,y,z,"--OVER--",0xFFBB0000);
			else
				g_system.DrawBMPText(offset_x+150,y,z,"--OVER--",0xFFFF0000);
		}

		delete [] tmp_str;
	}

	//フェードアウトα値を取得する
	BYTE CCOptionSelecter::GetFadeOut()
	{
		DWORD tmp = counter*4;
		if(state==0){
			if(tmp>255)return 255;
			return (BYTE)tmp;
		}
		else if(state==1){
			if(tmp>255)return 0;
			return (BYTE)(255-tmp);
		}
		return 0;
	}

	DWORD CCOptionSelecter::GetSettings()
	{
		DWORD ret = 0;
		DWORD k=0;
		CharOptionList::iterator ite;
		for(ite=list->begin();ite!=list->end();ite++,k++){
			if(enabled[k]){
				ret |= ite->flag;
			}
		}
		return ret;
	}

	void CCOptionSelecter::SetRandom()
	{
		m_current_favorite = rand()%(m_ref_cinfo->fav_opts.size()+1);
		if(m_current_favorite!=0)
		{
			//favorite設定でのランダム
			DWORD current_favorite = m_current_favorite;
			Initialize( m_ref_cinfo->fav_opts[m_current_favorite-1].opt );
			m_current_favorite = current_favorite;
			return;
		}

		//Freeランダム
		int k;
		for(k=0;k<32;k++){
			enabled[k]=FALSE;
		}
		current_point = maxpoint;

		std::list<CHARACTER_LOAD_OPTION> dlist;
		CharOptionList::iterator i2;
		for(i2=list->begin();i2!=list->end();i2++){
			dlist.push_back(*i2);
		}

		//srand(timeGetTime());

		DWORD ret=0;
		DWORD breaker=0;
		std::list<CHARACTER_LOAD_OPTION>::iterator i;
		while( current_point>0 && dlist.size()!=0)
		{
			for(i=dlist.begin();i!=dlist.end();i++)
			{
				if(i->point > current_point){//ポイントチェック
					dlist.erase(i);
					break;
				}
				else if(ret & i->exclusive){//競合チェック
					dlist.erase(i);
					break;
				}
				else if(rand()%4==0 )
				{
					if( (i->depends&ret)==i->depends )//依存チェック
					{
						//フラグ加算
						current_point -= i->point;
						ret |= i->flag;
						//gbl.ods("%s %d %d %X",i->name,i->point,current_point,i->flag);
						dlist.erase(i);


						break;
					}
				}
				else if(i->point<=0 && rand()%3)
				{
					dlist.erase(i);
					break;
				}
			}
			if(breaker++>1000){
				gbl.ods("CCOptionSelecter::SetRandom 強制ブレーク\n");
				break;
			}
		}

		//ネガティブオプション対策
		dlist.clear();
		for(i2=list->begin();i2!=list->end();i2++){
			dlist.push_back(*i2);
		}
		for(i=dlist.begin();i!=dlist.end();i++)
		{
			if(ret&i->flag)continue;
			if(i->point <= current_point && i->point>0){//ポイントチェック
				if(ret & i->exclusive){//競合チェック
				}
				else if( (i->depends&ret)==i->depends )//依存チェック
				{
					//フラグ加算
					current_point -= i->point;
					ret |= i->flag;
					//gbl.ods("%s %d %d %X",i->name,i->point,current_point,i->flag);
				}
			}
		}

		Initialize(ret);
		//return ret;
	}

	//0:"Free" , 0～:favorite設定名
	char* CCOptionSelecter::GetCurrentSetName()
	{
		if(m_current_favorite==0)return "Free";

		return m_ref_cinfo->fav_opts[ m_current_favorite-1 ].name;
	}

	//前回選択フラグに設定する
	void CCOptionSelecter::ApplyToPreviousSelect()
	{
		m_ref_cinfo->previous_favorite = m_current_favorite;
		m_ref_cinfo->previous_option = GetSettings();
	}


	/*****************************************************************
		キャラ選択オプションセレクター（単純化版）
	******************************************************************/

	CCSimpleOptionSelecter::CCSimpleOptionSelecter(CCL_CHARACTERINFO *cinfo,DWORD maxp)
	: CCOptionSelecter(cinfo, maxp)
	{

	}

	//パッド入力を処理する
	BOOL CCSimpleOptionSelecter::HandlePad(DWORD inputIndex)
	{
		counter++;
		CharOptionList::iterator list2=list->begin();

		DWORD input = g_input.GetKey(inputIndex,0);

		//↑↓入力処理
		if(input & KEYSTA_UP2){
			m_current_favorite--;
			if(m_current_favorite == 0xFFFFFFFF){
				m_current_favorite = m_ref_cinfo->fav_opts.size();
			}
			if(m_current_favorite != 0)
			{
				DWORD current_favorite = m_current_favorite;
				Initialize( m_ref_cinfo->fav_opts[m_current_favorite-1].opt );
				m_current_favorite = current_favorite;
			}
			//point over check
			if(current_point<0)
				commit_enabled=FALSE;
			else
				commit_enabled=TRUE;
		}
		else if(input & (KEYSTA_DOWN2 | KEYSTA_BB2)){				// Bでも操作可能に
			m_current_favorite++;
			if(m_current_favorite == m_ref_cinfo->fav_opts.size()+1){
				m_current_favorite = 0;
			}
			else
			{
				DWORD current_favorite = m_current_favorite;
				Initialize( m_ref_cinfo->fav_opts[m_current_favorite-1].opt );
				m_current_favorite = current_favorite;
			}
			//point over check
			if(current_point<0)
				commit_enabled=FALSE;
			else
				commit_enabled=TRUE;
		}

		DWORD curr_flag = GetSettings();

		DWORD key1 = g_input.GetKey(inputIndex,1);
		DWORD key2 = g_input.GetKey(inputIndex,2);
		DWORD key3 = g_input.GetKey(inputIndex,3);
		BOOL leftON  = (input&KEYSTA_ALEFT) && (key1&KEYSTA_ALEFT) && (key2&KEYSTA_ALEFT) && (key3&KEYSTA_ALEFT2);
		BOOL rightON = (input&KEYSTA_ARIGHT) && (key1&KEYSTA_ARIGHT) && (key2&KEYSTA_ARIGHT) && (key3&KEYSTA_ARIGHT2);

		//B入力処理(optset変更)
		if(input & KEYSTA_BB2)
		{
		}
		//optset設定から上下キーが入ったらFree
		/*if(m_current_favorite!=0){
			if(current_selected != list->size())
			{
				m_current_favorite=0;
			}
		}*/

		//C入力処理(ランダム)
		if( input&(KEYSTA_BC2) && !m_ref_cinfo->fav_opts.empty() )
		{
	/*		SetRandom();
			current_selected = list->size();//OK位置にカーソル移動
	*/		m_current_favorite = rand() % (m_ref_cinfo->fav_opts.size() +1);

			DWORD current_favorite = m_current_favorite;
			Initialize( m_ref_cinfo->fav_opts[m_current_favorite-1].opt );
			m_current_favorite = current_favorite;
			//point over check
			if(current_point<0)
				commit_enabled=FALSE;
			else
				commit_enabled=TRUE;
		}

		// ON/OFF
		if(input&(KEYSTA_BA2))
		{
			if((commit_enabled||g_config.IsLimiterCut()) && input&(KEYSTA_BA2)){//A入力処理(決定)
				state=1;
				counter=0;
				return TRUE;
			}
		}

		if( input&(KEYSTA_BD2) )
		{
			this->state=0xFFFFFFFF;
			return TRUE;
		}
		return FALSE;
	}



}

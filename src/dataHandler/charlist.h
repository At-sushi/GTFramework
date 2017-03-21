
/*!
*	@file
*	@brief キャラクター管理
*	ディレクトリからキャラクターのリストを作成して管理する。
*/

#pragma once

#include "gobject.h"//needs CHARACTER_LOAD_OPTION

namespace GTF
{

	/*!
	*	@ingroup global
	*/
	/*@{*/

	typedef std::vector<CHARACTER_LOAD_OPTION> CharOptionList;	//!< キャラクターオプション配列(STL)


	/*!
	*	@brief キャラクターのoptset.txt情報
	*/
	struct FAVORITE_OPTION
	{
		char name[32];	//!< オプション名
		DWORD opt;		//!< オプション値
	};
	typedef std::vector<FAVORITE_OPTION> FavoriteOptionList;	//!< キャラクターオプションセット配列(STL)

	/*! @brief キャラクターの情報 */
	struct CCL_CHARACTERINFO
	{
		char dir[64];				//!< どのディレクトリにあるか
		char name[32];				//!< 名前はなんというのか
		DWORD ver;					//!< dllのバージョン

		DWORD caps;					//!< キャラクターの能力（ストーリー可/不可、ネットワーク対応/非対応等）
	
		//DWORD byteCheck;			//ネットワーク対戦時のチェック
		CharOptionList options;		//!< オプション項目
		DWORD maxpoint;				//!< オプションポイント最大値
		DWORD previous_option;		//!< 前回選択されたオプション
		FavoriteOptionList fav_opts;//!< オプションセット(optset.txtから取得)
		UINT  previous_favorite;	//!< 前回選択オプション? 未使用っぽい

		CCL_CHARACTERINFO(){previous_option=0;previous_favorite=0;}
	};
	typedef std::vector<CCL_CHARACTERINFO> CCLCharacterInfoList;	//!< キャラクター情報配列(STL)


	/*!	@brief 認識失敗キャラクターの情報 */
	struct CCL_DAMEINFO
	{
		char dir[64];				//!< どのディレクトリにあるか
		char name[32];				//!< 名前はなんというのか
		DWORD dame;					//!< 駄目な理由
		DWORD ver;					//!< dllのバージョン
	};
	typedef std::vector<CCL_DAMEINFO> CCLDameInfoList;	//!< 認識失敗キャラクター配列(STL)

	#define CCL_DAME_NODLL		1//!< キャラクター認識失敗理由ID , action.dllの読み込みに失敗
	#define CCL_DAME_CANTGETFP	2//!< キャラクター認識失敗理由ID , 関数ポインタ取得に失敗
	#define CCL_DAME_FFAIL		3//!< キャラクター認識失敗理由ID , 関数がFALSEを返してきた
	#define CCL_DAME_OLDDLL		4//!< キャラクター認識失敗理由ID , バージョンチェックに失敗
	#define CCL_DAME_NEWDLL		5//!< キャラクター認識失敗理由ID , バージョンチェックに失敗(2)



	/*! @brief リング（キャラクターディレクトリ）情報 */
	struct CCL_RINGINFO
	{
		char name[64];							//!< 名前=ディレクトリ名
		std::vector<DWORD> ring2serialIndex;	//!< 直列に並んだ順番でのインデックスに変換
	};
	typedef std::vector<CCL_RINGINFO> CCLRingInfoList;	//!< キャラクターリングリスト(STL)



	class CCOptionSelecter;
	class CCSimpleOptionSelecter;

	/*! @brief キャラクターリスト管理クラス */
	class CCharacterList
	{
	public:
		CCharacterList();
		~CCharacterList(){Destroy();}
	
		void Initialize();
		void Destroy();

		//キャラクタの情報取得系
		int GetCharacterCount();				//!< 検索された全キャラクター数を返す。
		char* GetCharacterDir(UINT index);		//!< キャラクターのディレクトリパスを返す
		char* GetCharacterDir(UINT index,int ring);
		char* GetCharacterName(UINT index);		//!< キャラクターの名前を返す
		DWORD GetCharacterVer(UINT index);		//!< キャラクターのバージョンを返す
		int   FindCharacter(char *name);		//!< 名前で検索（なかった場合-1）
		DWORD GetCaps(UINT index);				//!< キャラクターのcapsフィールドを取得する

		//リング情報取得系
		int GetRingNum();						//!< リングの数を返す
		char* GetRingName(UINT index);			//!< リングの名前（ディレクトリ名）を返す
		int GetCharacterCountRing(UINT index);	//!< 指定インデックスのリングで検索されたキャラクター数を返す
		DWORD RingIndex2SerialIndex(UINT ring,UINT index);//!< リング内でのインデックスを、全体でのインデックスに変換する

		//認識失敗キャラクターの情報
		int GetDameCharCount();					//!< 認識失敗キャラクターの数を返す
		char* GetDameCharDir(UINT index);		//!< 認識失敗キャラクターのディレクトリを返す
		char* GetDameCharName(UINT index);		//!< 認識失敗キャラクターの名前を返す
		DWORD GetDameCharReas(UINT index);		//!< 認識失敗キャラクターの認識失敗理由を返す
		DWORD GetDameCharVer(UINT index);		//!< 認識失敗キャラクターのバージョンを返す

	/*	//ネトワーク関係
		BOOL GetNetCharacterInfo(NETMESSAGE_INDICATE_CONNECT *dat);
		BOOL CheckNetCharacterList(NETMESSAGE_INDICATE_CONNECT *dat);
		void InitializeNetCharacterIDList();
		void SetNetCharacterIDList(NETMESSAGE_BODY_CHARACTER_ID* dat);
		DWORD GetRandomNetChar();
		DWORD GetIndexFromNetID(DWORD netid,BOOL exitWhenError=FALSE);
		DWORD GetByteCheck(DWORD index){return(infolist[index].byteCheck);}
		DWORD GetNetIDFromIndex(DWORD index);*/

		//オプション関係
		CCOptionSelecter* GetOptionSelecter(DWORD cindex);
		void SetPreviousOption(DWORD index,DWORD opt){infolist[index].previous_option=opt;}
		DWORD GetRandomOption(DWORD index);
		void  CorrectOption(UINT cindex,DWORD *opt);
		void LoadFavoriteOptions(char* dir,FavoriteOptionList& list);
		CCSimpleOptionSelecter* GetSimpleOptionSelecter(DWORD cindex);

	private:
		//Initializeから使用される
		void InitializeRingList();				//!< ディレクトリを検索し、リングのディレクトリ名リストを構築する
		void InitializeRing(DWORD ringindex);	//!< ディレクトリを検索する）
		BOOL VerifyCharacterDir(char *dir,DWORD ringindex);//!< キャラクタディレクトリの正当性を検証する

	private:
		CCLCharacterInfoList infolist;	//!< キャラクタリスト
		CCLRingInfoList ringlist;		//!< リングリスト
		CCLDameInfoList damelist;		//!< ディレクトリはあったけどverifyに失敗したリスト
	};



	class CTOptionSelecterBase;	//!< オプション選択ベースクラス
	class CTOptionSelecter;		//!< オプション選択クラス（VSモード時）
	class COptionSelecter;		//!< オプション選択クラス（ストーリーモード時）

	/*!
	*	@brief オプション選択データ管理クラス
	*	@sa CTOptionSelecter
	*
	*	ver0.90x で使ってた古いクラス。CTOptionSelecterはこいつを内部にもつ。
	*/
	class CCOptionSelecter
	{
	friend class CTOptionSelecterBase;
	friend class CTOptionSelecter;
	friend class CTOptionSelecterStory;
	friend class CTSimpleOptionSelecter;
	public:
		CCOptionSelecter(CCL_CHARACTERINFO *info,DWORD maxp);
		void Initialize(DWORD ini_opt);
		virtual BOOL HandlePad(DWORD input);
		void Draw();
		BYTE GetFadeOut();
		DWORD GetSettings();
		void SetOffset(int val){offset_x=val;}
		void SetZ(float val){z=val;}

		char* GetCurrentSetName();						//!< 0:"Free" , 0～:favorite設定名
		void ApplyToPreviousSelect();					//!< 前回選択フラグに設定する

	protected:
		void TurnOffDependFlags( DWORD flag );			//!< flagに依存するフラグを全てOFF
		void TurnOffExclisiveFlags( DWORD ex_flag );	//!< flagと競合するフラグを全てOFF
		void SetRandom();								//!< ランダムセット

		DWORD counter;					//!< 描画用カウンタ？
		DWORD state;					//!< 状態
		CharOptionList* list;			//!< オプションリスト
		DWORD current_selected;			//!< 選択中の項目
		int current_point;				//!< 現在残りポイント
		BOOL enabled[32];				//!< 各オプションON/OFFフラグ
		BOOL commit_enabled;			//!< 決定可能フラグ
		DWORD maxpoint;					//!< ポイント限界値
		int offset_x;					//!< 描画Xオフセット値
		float z;						//!< 描画Z値

		CCL_CHARACTERINFO *m_ref_cinfo;	//!< キャラクタ情報(参照)
		DWORD m_current_favorite;		//!< 現在の設定、0:Free , 1～:favolite0～
	};

	/*!
	*	@brief 単純化版用オプション選択データ管理クラス
	*	@sa CTSimpleOptionSelecter
	*
	*	CTSimpleOptionSelecter内部で使用
	*	CCOptionSelecterとほぼ同じですが、操作系が一部変わっています。
	*/
	class CCSimpleOptionSelecter : public CCOptionSelecter
	{
	friend class CTSimpleOptionSelecter;
	public:
		CCSimpleOptionSelecter(CCL_CHARACTERINFO *info,DWORD maxp);
		BOOL HandlePad(DWORD input);

		DWORD GetFavorite() { return 	m_current_favorite; }
		DWORD GetState() { return 	state; }
	};

	/*@}*/

}

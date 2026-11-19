#include "KioskPlaces.h"

#include "KioskData.h"
#include "KioskLoc.h"
#include "Widgets/Components/SKioskIcon.h"

namespace
{
	using C = EKioskPlaceCategory;

	struct FPlaceRow
	{
		const TCHAR* Name;
		EKioskPlaceCategory Category;
		const TCHAR* Station;
		const TCHAR* Keywords; // comma separated
	};

	// Nearest MRT station for each place. Keywords are alternate names and things found there.
	const FPlaceRow GPlaceRows[] = {
		// Attractions
		{ TEXT("Gardens by the Bay"), C::Attraction, TEXT("Bayfront"), TEXT("supertree grove,supertrees,flower dome,cloud forest,gardens bay,garden by the bay,gbtb") },
		{ TEXT("Marina Bay Sands"), C::Attraction, TEXT("Bayfront"), TEXT("mbs,casino,skypark,shoppes,marina sands,infinity pool") },
		{ TEXT("Merlion"), C::Attraction, TEXT("Raffles Place"), TEXT("merlion park,merlin,statue") },
		{ TEXT("Sentosa"), C::Attraction, TEXT("HarbourFront"), TEXT("sentosa island,beach,siloso,palawan,cable car,sentosa express") },
		{ TEXT("Universal Studios Singapore"), C::Attraction, TEXT("HarbourFront"), TEXT("uss,universal,theme park,resorts world sentosa,rws") },
		{ TEXT("S.E.A. Aquarium"), C::Attraction, TEXT("HarbourFront"), TEXT("sea aquarium,aquarium,oceanarium") },
		{ TEXT("Mount Faber"), C::Attraction, TEXT("HarbourFront"), TEXT("mount faber park,faber peak,cable car") },
		{ TEXT("Singapore Zoo"), C::Attraction, TEXT("Khatib"), TEXT("zoo,mandai,river wonders,night safari,bird paradise,wildlife") },
		{ TEXT("Singapore Botanic Gardens"), C::Attraction, TEXT("Botanic Gardens"), TEXT("botanic,botanical garden,orchid garden,national orchid garden") },
		{ TEXT("Singapore Flyer"), C::Attraction, TEXT("Promenade"), TEXT("flyer,ferris wheel") },
		{ TEXT("Esplanade Theatres"), C::Attraction, TEXT("Esplanade"), TEXT("esplanade,durian,theatre,concert hall") },
		{ TEXT("Clarke Quay"), C::Attraction, TEXT("Clarke Quay"), TEXT("riverside,nightlife,river cruise") },
		{ TEXT("Boat Quay"), C::Attraction, TEXT("Raffles Place"), TEXT("river,riverside") },
		{ TEXT("Chinatown Heritage Centre"), C::Attraction, TEXT("Chinatown"), TEXT("heritage,pagoda street,temple street") },
		{ TEXT("Buddha Tooth Relic Temple"), C::Attraction, TEXT("Chinatown"), TEXT("temple,buddha tooth") },
		{ TEXT("Sri Mariamman Temple"), C::Attraction, TEXT("Chinatown"), TEXT("hindu temple,mariamman") },
		{ TEXT("Little India"), C::Area, TEXT("Little India"), TEXT("tekka,serangoon road,indian") },
		{ TEXT("Fort Canning Park"), C::Attraction, TEXT("Dhoby Ghaut"), TEXT("fort canning,park,hill") },
		{ TEXT("Marina Barrage"), C::Attraction, TEXT("Marina South Pier"), TEXT("barrage,kite,rooftop garden") },
		{ TEXT("East Coast Park"), C::Attraction, TEXT("Marine Parade"), TEXT("beach,cycling,east coast,bbq") },
		{ TEXT("Singapore Expo"), C::Attraction, TEXT("Expo"), TEXT("expo,convention,exhibition") },
		{ TEXT("Bukit Timah Nature Reserve"), C::Attraction, TEXT("Hume"), TEXT("bukit timah hill,hillview,hiking,nature,hume") },
		{ TEXT("Kent Ridge Park"), C::Attraction, TEXT("Kent Ridge"), TEXT("hort park,southern ridges,henderson waves") },
		{ TEXT("Pasir Ris Park"), C::Attraction, TEXT("Pasir Ris"), TEXT("pasir ris beach,park,mangrove") },
		{ TEXT("Bishan-AMK Park"), C::Attraction, TEXT("Bishan"), TEXT("bishan park,park") },
		{ TEXT("Science Centre Singapore"), C::Attraction, TEXT("Jurong East"), TEXT("science centre,science museum") },
		{ TEXT("Woodlands Checkpoint"), C::Area, TEXT("Woodlands"), TEXT("causeway,johor,malaysia,jb,border,customs") },
		// Shopping
		{ TEXT("ION Orchard"), C::Shopping, TEXT("Orchard"), TEXT("ion,orchard shopping") },
		{ TEXT("Orchard Road"), C::Shopping, TEXT("Orchard"), TEXT("shopping street,ngee ann city,takashimaya,paragon,wisma") },
		{ TEXT("VivoCity"), C::Shopping, TEXT("HarbourFront"), TEXT("vivo city,vivo,mall") },
		{ TEXT("Suntec City"), C::Shopping, TEXT("Promenade"), TEXT("suntec,convention centre") },
		{ TEXT("Marina Square"), C::Shopping, TEXT("Promenade"), TEXT("marina sq,millenia walk") },
		{ TEXT("Bugis Junction"), C::Shopping, TEXT("Bugis"), TEXT("bugis plus,bugis street,bugis village") },
		{ TEXT("Mustafa Centre"), C::Shopping, TEXT("Little India"), TEXT("mustafa,24 hour,24hr") },
		{ TEXT("Chinatown Point"), C::Shopping, TEXT("Chinatown"), TEXT("china town point,peoples park") },
		{ TEXT("Plaza Singapura"), C::Shopping, TEXT("Dhoby Ghaut"), TEXT("plaza sing,the cathay,atrium") },
		{ TEXT("Tampines Mall"), C::Shopping, TEXT("Tampines"), TEXT("tampines hub,century square,tampines one") },
		{ TEXT("Jurong Point"), C::Shopping, TEXT("Boon Lay"), TEXT("jp,jurong pt") },
		{ TEXT("JEM and Westgate"), C::Shopping, TEXT("Jurong East"), TEXT("jem,westgate,imm,jurong east mall") },
		{ TEXT("Causeway Point"), C::Shopping, TEXT("Woodlands"), TEXT("causeway,woodlands mall") },
		{ TEXT("Waterway Point"), C::Shopping, TEXT("Punggol"), TEXT("waterway,punggol mall") },
		{ TEXT("NEX Mall"), C::Shopping, TEXT("Serangoon"), TEXT("nex,serangoon mall") },
		{ TEXT("Paya Lebar Quarter"), C::Shopping, TEXT("Paya Lebar"), TEXT("plq,paya lebar square") },
		{ TEXT("AMK Hub"), C::Shopping, TEXT("Ang Mo Kio"), TEXT("amk,ang mo kio mall") },
		{ TEXT("Junction 8"), C::Shopping, TEXT("Bishan"), TEXT("j8,bishan junction") },
		// Hospitals
		{ TEXT("Singapore General Hospital"), C::Hospital, TEXT("Outram Park"), TEXT("sgh,general hospital,national cancer centre,national dental centre") },
		{ TEXT("Tan Tock Seng Hospital"), C::Hospital, TEXT("Novena"), TEXT("ttsh,tan tock seng,novena hospital") },
		{ TEXT("National University Hospital"), C::Hospital, TEXT("Kent Ridge"), TEXT("nuh,nus hospital") },
		{ TEXT("Changi General Hospital"), C::Hospital, TEXT("Simei"), TEXT("cgh,changi hospital") },
		{ TEXT("KK Women's and Children's Hospital"), C::Hospital, TEXT("Little India"), TEXT("kkh,kk hospital,women and children,children hospital") },
		{ TEXT("Khoo Teck Puat Hospital"), C::Hospital, TEXT("Yishun"), TEXT("ktph,yishun hospital") },
		{ TEXT("Mount Elizabeth Hospital"), C::Hospital, TEXT("Orchard"), TEXT("mount e,mt elizabeth") },
		// Education
		{ TEXT("National University of Singapore"), C::Education, TEXT("Kent Ridge"), TEXT("nus,kent ridge campus") },
		{ TEXT("Nanyang Technological University"), C::Education, TEXT("Boon Lay"), TEXT("ntu,nanyang,ntu campus") },
		{ TEXT("Singapore Management University"), C::Education, TEXT("Dhoby Ghaut"), TEXT("smu,bras basah") },
		// Airport
		{ TEXT("Changi Airport"), C::Airport, TEXT("Changi Airport"), TEXT("changi,airport,flight,flights,terminal,terminal 1,terminal 2,terminal 3,terminal 4,departure,plane") },
		{ TEXT("Jewel Changi Airport"), C::Airport, TEXT("Changi Airport"), TEXT("jewel,rain vortex,canopy park") },
		// Food
		{ TEXT("Lau Pa Sat"), C::Food, TEXT("Raffles Place"), TEXT("lau pasat,telok ayer market,satay street,hawker") },
		{ TEXT("Maxwell Food Centre"), C::Food, TEXT("Chinatown"), TEXT("maxwell,hawker,tian tian chicken rice,chinatown complex") },
		{ TEXT("Newton Food Centre"), C::Food, TEXT("Newton"), TEXT("newton hawker,crazy rich asians,hawker") },
		{ TEXT("Tekka Centre"), C::Food, TEXT("Little India"), TEXT("tekka market,hawker,indian food") },
		{ TEXT("East Coast Lagoon Food Village"), C::Food, TEXT("Marine Parade"), TEXT("lagoon,satay,seafood") },
		{ TEXT("Gluttons Bay"), C::Food, TEXT("Esplanade"), TEXT("satay,hawker,esplanade food") },
	};

	/** What a category means when typed ("shopping", "hospital") so a goal word lists every match. */
	const TCHAR* CategoryKeywords(EKioskPlaceCategory Category)
	{
		switch (Category)
		{
		case C::Attraction: return TEXT("attraction,tourist,sightseeing,visit");
		case C::Shopping: return TEXT("shopping,mall,shop");
		case C::Hospital: return TEXT("hospital,doctor,clinic,medical");
		case C::Airport: return TEXT("flight,fly");
		case C::Education: return TEXT("university,school,campus,study");
		case C::Food: return TEXT("food,eat,hawker,restaurant,lunch,dinner");
		default: return TEXT("");
		}
	}

	TArray<FKioskPlace> BuildPlaces()
	{
		TArray<FKioskPlace> Result;
		for (const FPlaceRow& Row : GPlaceRows)
		{
			FKioskPlace Place;
			Place.Name = Row.Name;
			Place.Category = Row.Category;
			Place.Station = FKioskData::FindStation(Row.Station);
			check(Place.Station != INDEX_NONE);
			TArray<FString> Extra;
			FString(Row.Keywords).ParseIntoArray(Place.Keywords, TEXT(","));
			FString(CategoryKeywords(Row.Category)).ParseIntoArray(Extra, TEXT(","));
			Place.Keywords.Append(Extra);
			Result.Add(Place);
		}
		return Result;
	}

	const FKioskAlias GAliasRows[] = {
		// Spelling variants
		{ TEXT("harbor front"), TEXT("HarbourFront") },
		{ TEXT("harbour front"), TEXT("HarbourFront") },
		{ TEXT("harbor"), TEXT("HarbourFront") },
		{ TEXT("habourfront"), TEXT("HarbourFront") },
		{ TEXT("dhobi ghaut"), TEXT("Dhoby Ghaut") },
		{ TEXT("dobi ghaut"), TEXT("Dhoby Ghaut") },
		{ TEXT("dobby ghaut"), TEXT("Dhoby Ghaut") },
		{ TEXT("dhobi"), TEXT("Dhoby Ghaut") },
		{ TEXT("dhoby"), TEXT("Dhoby Ghaut") },
		{ TEXT("dobi"), TEXT("Dhoby Ghaut") },
		{ TEXT("bay front"), TEXT("Bayfront") },
		{ TEXT("china town"), TEXT("Chinatown") },
		{ TEXT("chinese town"), TEXT("Chinatown") },
		{ TEXT("rafles place"), TEXT("Raffles Place") },
		{ TEXT("raffles"), TEXT("Raffles Place") },
		{ TEXT("botanical gardens"), TEXT("Botanic Gardens") },
		{ TEXT("jurong"), TEXT("Jurong East") },
		{ TEXT("woodland"), TEXT("Woodlands") },
		{ TEXT("airport"), TEXT("Changi Airport") },
		{ TEXT("changi"), TEXT("Changi Airport") },
		{ TEXT("lapangan terbang"), TEXT("Changi Airport") },
		{ TEXT("kebun botani"), TEXT("Singapore Botanic Gardens") },
		{ TEXT("zoo"), TEXT("Singapore Zoo") },
		{ TEXT("nus"), TEXT("National University of Singapore") },
		// Pinyin, for people who read Chinese but type on this Latin keyboard
		{ TEXT("gangwan"), TEXT("HarbourFront") },
		{ TEXT("shengtaosha"), TEXT("Sentosa") },
		{ TEXT("niuchesui"), TEXT("Chinatown") },
		{ TEXT("wujie"), TEXT("Orchard") },
		{ TEXT("zhangyi jichang"), TEXT("Changi Airport") },
		{ TEXT("zhangyi"), TEXT("Changi Airport") },
		{ TEXT("jichang"), TEXT("Changi Airport") },
		{ TEXT("binhaiwan huayuan"), TEXT("Gardens by the Bay") },
		{ TEXT("yuweishi"), TEXT("Merlion") },
		{ TEXT("xiaoyindu"), TEXT("Little India") },
		{ TEXT("wujishi"), TEXT("Bugis") },
		{ TEXT("dongwuyuan"), TEXT("Singapore Zoo") },
		{ TEXT("zhiwuyuan"), TEXT("Singapore Botanic Gardens") },
		{ TEXT("yifengcheng"), TEXT("VivoCity") },
	};

	struct FNameRow
	{
		const TCHAR* English;
		const TCHAR* Chinese;
		const TCHAR* Malay;
		const TCHAR* Tamil;
	};

	// Empty string = no separate name in that language. Stations first, then places.
	const FNameRow GNames[] = {
		{ TEXT("Woodlands North"), TEXT("兀兰北"), TEXT(""), TEXT("உட்லண்ட்ஸ் நார்த்") },
		{ TEXT("Woodlands"), TEXT("兀兰"), TEXT(""), TEXT("உட்லண்ட்ஸ்") },
		{ TEXT("Marsiling"), TEXT("马西岭"), TEXT(""), TEXT("மார்சிலிங்") },
		{ TEXT("Yishun"), TEXT("义顺"), TEXT(""), TEXT("ஈசூன்") },
		{ TEXT("Ang Mo Kio"), TEXT("宏茂桥"), TEXT(""), TEXT("ஆங் மோ கியோ") },
		{ TEXT("Bishan"), TEXT("碧山"), TEXT(""), TEXT("பிஷான்") },
		{ TEXT("Marymount"), TEXT("玛丽蒙"), TEXT(""), TEXT("மேரிமவுண்ட்") },
		{ TEXT("Caldecott"), TEXT("加利谷"), TEXT(""), TEXT("கால்டிகாட்") },
		{ TEXT("Botanic Gardens"), TEXT("植物园"), TEXT("Taman Botani"), TEXT("பொட்டானிக் கார்டன்ஸ்") },
		{ TEXT("Newton"), TEXT("纽顿"), TEXT(""), TEXT("நியூட்டன்") },
		{ TEXT("Orchard"), TEXT("乌节"), TEXT(""), TEXT("ஆர்ச்சர்ட்") },
		{ TEXT("Dhoby Ghaut"), TEXT("多美歌"), TEXT(""), TEXT("டோபி காட்") },
		{ TEXT("Little India"), TEXT("小印度"), TEXT(""), TEXT("லிட்டில் இந்தியா") },
		{ TEXT("Bugis"), TEXT("武吉士"), TEXT(""), TEXT("புகிஸ்") },
		{ TEXT("City Hall"), TEXT("政府大厦"), TEXT(""), TEXT("சிட்டி ஹால்") },
		{ TEXT("Raffles Place"), TEXT("莱佛士坊"), TEXT(""), TEXT("ராஃபிள்ஸ் பிளேஸ்") },
		{ TEXT("Marina Bay"), TEXT("滨海湾"), TEXT(""), TEXT("மரினா பே") },
		{ TEXT("Marina South Pier"), TEXT("滨海南码头"), TEXT(""), TEXT("மரினா சவுத் பியர்") },
		{ TEXT("Promenade"), TEXT("宜康"), TEXT(""), TEXT("ப்ரொமனேட்") },
		{ TEXT("Outram Park"), TEXT("欧南园"), TEXT(""), TEXT("அவுட்ரம் பார்க்") },
		{ TEXT("Chinatown"), TEXT("牛车水"), TEXT(""), TEXT("சைனாடவுன்") },
		{ TEXT("HarbourFront"), TEXT("港湾"), TEXT(""), TEXT("ஹார்பர்ஃப்ரண்ட்") },
		{ TEXT("Hume"), TEXT("休姆"), TEXT(""), TEXT("ஹியூம்") },
		{ TEXT("Beauty World"), TEXT("美世界"), TEXT(""), TEXT("பியூட்டி வேர்ல்ட்") },
		{ TEXT("Bukit Panjang"), TEXT("武吉班让"), TEXT(""), TEXT("புக்கிட் பாஞ்சாங்") },
		{ TEXT("Buona Vista"), TEXT("波那维斯达"), TEXT(""), TEXT("புவானா விஸ்டா") },
		{ TEXT("Jurong East"), TEXT("裕廊东"), TEXT(""), TEXT("ஜூரோங் ஈஸ்ட்") },
		{ TEXT("Boon Lay"), TEXT("文礼"), TEXT(""), TEXT("பூன் லே") },
		{ TEXT("Serangoon"), TEXT("实龙岗"), TEXT(""), TEXT("செராங்கூன்") },
		{ TEXT("Sengkang"), TEXT("盛港"), TEXT(""), TEXT("செங்காங்") },
		{ TEXT("Punggol"), TEXT("榜鹅"), TEXT(""), TEXT("பொங்கோல்") },
		{ TEXT("Paya Lebar"), TEXT("巴耶利巴"), TEXT(""), TEXT("பாயா லேபார்") },
		{ TEXT("MacPherson"), TEXT("麦波申"), TEXT(""), TEXT("மெக்பெர்சன்") },
		{ TEXT("Tanah Merah"), TEXT("丹那美拉"), TEXT(""), TEXT("தானா மேரா") },
		{ TEXT("Tampines"), TEXT("淡滨尼"), TEXT(""), TEXT("தெம்பனீஸ்") },
		{ TEXT("Pasir Ris"), TEXT("巴西立"), TEXT(""), TEXT("பாசிர் ரிஸ்") },
		{ TEXT("Expo"), TEXT("博览"), TEXT(""), TEXT("எக்ஸ்போ") },
		{ TEXT("Changi Airport"), TEXT("樟宜机场"), TEXT("Lapangan Terbang Changi"), TEXT("சாங்கி விமான நிலையம்") },
		{ TEXT("Gardens by the Bay"), TEXT("滨海湾花园"), TEXT(""), TEXT("கார்டன்ஸ் பை தி பே") },
		{ TEXT("Marine Parade"), TEXT("马林百列"), TEXT(""), TEXT("மரின் பரேட்") },
		{ TEXT("Marine Terrace"), TEXT("马林台"), TEXT(""), TEXT("மரின் டெரஸ்") },
		{ TEXT("Bayshore"), TEXT("碧湾"), TEXT(""), TEXT("பேஷோர்") },
		{ TEXT("Bayfront"), TEXT("海湾舫"), TEXT(""), TEXT("பேஃப்ரண்ட்") },
		{ TEXT("Kent Ridge"), TEXT("肯特岗"), TEXT(""), TEXT("கெண்ட் ரிட்ஜ்") },
		{ TEXT("Khatib"), TEXT("卡迪"), TEXT(""), TEXT("காத்திப்") },
		{ TEXT("Novena"), TEXT("诺维娜"), TEXT(""), TEXT("நோவீனா") },
		{ TEXT("Clarke Quay"), TEXT("克拉码头"), TEXT(""), TEXT("கிளார்க் கீ") },
		{ TEXT("Esplanade"), TEXT("滨海中心"), TEXT(""), TEXT("எஸ்பிளனேட்") },
		{ TEXT("Simei"), TEXT("四美"), TEXT(""), TEXT("சிமே") },
		{ TEXT("Bukit Batok"), TEXT("武吉巴督"), TEXT(""), TEXT("புக்கிட் பாத்தோக்") },
		{ TEXT("Choa Chu Kang"), TEXT("蔡厝港"), TEXT(""), TEXT("சுவா சூ காங்") },
		{ TEXT("Tuas Link"), TEXT("大士连路"), TEXT(""), TEXT("டுவாஸ் லிங்க்") },
		{ TEXT("Tuas West Road"), TEXT("大士西路"), TEXT(""), TEXT("டுவாஸ் வெஸ்ட் ரோடு") },
		{ TEXT("Tuas Crescent"), TEXT("大士弯道"), TEXT(""), TEXT("டுவாஸ் கிரெசென்ட்") },
		{ TEXT("Gul Circle"), TEXT("古勒环"), TEXT(""), TEXT("குல் சர்க்கிள்") },
		{ TEXT("Joo Koon"), TEXT("裕群"), TEXT(""), TEXT("ஜூ கூன்") },
		{ TEXT("Pioneer"), TEXT("先驱"), TEXT(""), TEXT("பயனீர்") },
		{ TEXT("Lakeside"), TEXT("湖畔"), TEXT(""), TEXT("லேக்சைட்") },
		{ TEXT("Chinese Garden"), TEXT("裕华园"), TEXT(""), TEXT("சைனீஸ் கார்டன்") },
		{ TEXT("Clementi"), TEXT("金文泰"), TEXT(""), TEXT("கிளிமெண்டி") },
		{ TEXT("Dover"), TEXT("杜弗"), TEXT(""), TEXT("டோவர்") },
		{ TEXT("Commonwealth"), TEXT("联邦"), TEXT(""), TEXT("காமன்வெல்த்") },
		{ TEXT("Queenstown"), TEXT("女皇镇"), TEXT(""), TEXT("குவீன்ஸ்டவுன்") },
		{ TEXT("Redhill"), TEXT("红山"), TEXT(""), TEXT("ரெட்ஹில்") },
		{ TEXT("Tiong Bahru"), TEXT("中峇鲁"), TEXT(""), TEXT("தியோங் பாரு") },
		{ TEXT("Tanjong Pagar"), TEXT("丹戎巴葛"), TEXT(""), TEXT("தஞ்சோங் பகார்") },
		{ TEXT("Lavender"), TEXT("劳明达"), TEXT(""), TEXT("லேவண்டர்") },
		{ TEXT("Kallang"), TEXT("加冷"), TEXT(""), TEXT("கல்லாங்") },
		{ TEXT("Aljunied"), TEXT("阿裕尼"), TEXT(""), TEXT("அல்ஜுனிட்") },
		{ TEXT("Eunos"), TEXT("友诺士"), TEXT(""), TEXT("யூனோஸ்") },
		{ TEXT("Kembangan"), TEXT("肯巴昌"), TEXT(""), TEXT("கெம்பாங்கான்") },
		{ TEXT("Bedok"), TEXT("勿洛"), TEXT(""), TEXT("பெடோக்") },
		{ TEXT("Bukit Gombak"), TEXT("武吉甘柏"), TEXT(""), TEXT("புக்கிட் கொம்பாக்") },
		{ TEXT("Yew Tee"), TEXT("油池"), TEXT(""), TEXT("யூ டீ") },
		{ TEXT("Kranji"), TEXT("克兰芝"), TEXT(""), TEXT("கிராஞ்சி") },
		{ TEXT("Admiralty"), TEXT("海军部"), TEXT(""), TEXT("அட்மிரால்ட்டி") },
		{ TEXT("Sembawang"), TEXT("三巴旺"), TEXT(""), TEXT("செம்பவாங்") },
		{ TEXT("Canberra"), TEXT("坎贝拉"), TEXT(""), TEXT("கான்பெரா") },
		{ TEXT("Yio Chu Kang"), TEXT("杨厝港"), TEXT(""), TEXT("இயோ சூ காங்") },
		{ TEXT("Braddell"), TEXT("布拉德"), TEXT(""), TEXT("பிராடல்") },
		{ TEXT("Toa Payoh"), TEXT("大巴窑"), TEXT(""), TEXT("தோ பாயோ") },
		{ TEXT("Somerset"), TEXT("索美塞"), TEXT(""), TEXT("சோமர்செட்") },
		{ TEXT("Bras Basah"), TEXT("百胜"), TEXT(""), TEXT("பிராஸ் பசா") },
		{ TEXT("Nicoll Highway"), TEXT("尼诰大道"), TEXT(""), TEXT("நிக்கல் நெடுஞ்சாலை") },
		{ TEXT("Stadium"), TEXT("体育场"), TEXT(""), TEXT("விளையாட்டரங்கம்") },
		{ TEXT("Mountbatten"), TEXT("蒙巴登"), TEXT(""), TEXT("மவுண்ட்பேட்டன்") },
		{ TEXT("Dakota"), TEXT("达科达"), TEXT(""), TEXT("டகோட்டா") },
		{ TEXT("Tai Seng"), TEXT("大成"), TEXT(""), TEXT("தை செங்") },
		{ TEXT("Bartley"), TEXT("巴特礼"), TEXT(""), TEXT("பார்ட்லி") },
		{ TEXT("Lorong Chuan"), TEXT("罗弄泉"), TEXT(""), TEXT("லோரோங் சுவான்") },
		{ TEXT("Farrer Road"), TEXT("花拉路"), TEXT(""), TEXT("ஃபேரர் ரோடு") },
		{ TEXT("Holland Village"), TEXT("荷兰村"), TEXT(""), TEXT("ஹாலந்து வில்லேஜ்") },
		{ TEXT("one-north"), TEXT("纬壹科技城"), TEXT(""), TEXT("ஒன்-நார்த்") },
		{ TEXT("Haw Par Villa"), TEXT("虎豹别墅"), TEXT(""), TEXT("ஹா பார் வில்லா") },
		{ TEXT("Pasir Panjang"), TEXT("巴西班让"), TEXT(""), TEXT("பாசிர் பாஞ்சாங்") },
		{ TEXT("Labrador Park"), TEXT("拉柏多公园"), TEXT(""), TEXT("லாப்ரடோர் பார்க்") },
		{ TEXT("Telok Blangah"), TEXT("直落布兰雅"), TEXT(""), TEXT("தெலுக் பிளாங்கா") },
		{ TEXT("Keppel"), TEXT("吉宝"), TEXT(""), TEXT("கெப்பல்") },
		{ TEXT("Cantonment"), TEXT("康东门"), TEXT(""), TEXT("கேண்டன்மென்ட்") },
		{ TEXT("Prince Edward Road"), TEXT("爱德华太子路"), TEXT(""), TEXT("பிரின்ஸ் எட்வர்ட் ரோடு") },
		{ TEXT("Cashew"), TEXT("腰果"), TEXT(""), TEXT("காஷ்யூ") },
		{ TEXT("Hillview"), TEXT("山景"), TEXT(""), TEXT("ஹில்வியூ") },
		{ TEXT("King Albert Park"), TEXT("景亚拔公园"), TEXT(""), TEXT("கிங் ஆல்பர்ட் பார்க்") },
		{ TEXT("Sixth Avenue"), TEXT("第六道"), TEXT(""), TEXT("சிக்ஸ்த் அவென்யூ") },
		{ TEXT("Tan Kah Kee"), TEXT("陈嘉庚"), TEXT(""), TEXT("டான் கா கீ") },
		{ TEXT("Rochor"), TEXT("梧槽"), TEXT(""), TEXT("ரோச்சோர்") },
		{ TEXT("Fort Canning"), TEXT("福康宁"), TEXT(""), TEXT("ஃபோர்ட் கேனிங்") },
		{ TEXT("Bencoolen"), TEXT("明古连"), TEXT(""), TEXT("பென்கூலன்") },
		{ TEXT("Jalan Besar"), TEXT("惹兰勿刹"), TEXT(""), TEXT("ஜாலான் பெசார்") },
		{ TEXT("Bendemeer"), TEXT("明地迈"), TEXT(""), TEXT("பெண்டிமீர்") },
		{ TEXT("Geylang Bahru"), TEXT("芽笼峇鲁"), TEXT(""), TEXT("கேலாங் பாரு") },
		{ TEXT("Mattar"), TEXT("玛达"), TEXT(""), TEXT("மாத்தார்") },
		{ TEXT("Ubi"), TEXT("乌美"), TEXT(""), TEXT("உபி") },
		{ TEXT("Kaki Bukit"), TEXT("加基武吉"), TEXT(""), TEXT("காகி புக்கிட்") },
		{ TEXT("Bedok North"), TEXT("勿洛北"), TEXT(""), TEXT("பெடோக் நார்த்") },
		{ TEXT("Bedok Reservoir"), TEXT("勿洛蓄水池"), TEXT(""), TEXT("பெடோக் நீர்த்தேக்கம்") },
		{ TEXT("Tampines West"), TEXT("淡滨尼西"), TEXT(""), TEXT("தெம்பனீஸ் வெஸ்ட்") },
		{ TEXT("Tampines East"), TEXT("淡滨尼东"), TEXT(""), TEXT("தெம்பனீஸ் ஈஸ்ட்") },
		{ TEXT("Upper Changi"), TEXT("樟宜上段"), TEXT(""), TEXT("அப்பர் சாங்கி") },
		{ TEXT("Xilin"), TEXT("溪林"), TEXT(""), TEXT("ஷிலின்") },
		{ TEXT("Downtown"), TEXT("市中心"), TEXT(""), TEXT("டவுன்டவுன்") },
		{ TEXT("Telok Ayer"), TEXT("直落亚逸"), TEXT(""), TEXT("தெலுக் ஆயர்") },
		{ TEXT("Stevens"), TEXT("史蒂芬"), TEXT(""), TEXT("ஸ்டீவன்ஸ்") },
		{ TEXT("Sungei Bedok"), TEXT("勿洛河"), TEXT(""), TEXT("சுங்கை பெடோக்") },
		{ TEXT("Farrer Park"), TEXT("花拉公园"), TEXT(""), TEXT("ஃபேரர் பார்க்") },
		{ TEXT("Boon Keng"), TEXT("文庆"), TEXT(""), TEXT("பூன் கெங்") },
		{ TEXT("Potong Pasir"), TEXT("波东巴西"), TEXT(""), TEXT("பொத்தோங் பாசிர்") },
		{ TEXT("Woodleigh"), TEXT("兀里"), TEXT(""), TEXT("உட்லீ") },
		{ TEXT("Kovan"), TEXT("高文"), TEXT(""), TEXT("கோவன்") },
		{ TEXT("Hougang"), TEXT("后港"), TEXT(""), TEXT("ஹவ்காங்") },
		{ TEXT("Buangkok"), TEXT("万国"), TEXT(""), TEXT("புவாங்கோக்") },
		{ TEXT("Punggol Coast"), TEXT("榜鹅海岸"), TEXT(""), TEXT("பொங்கோல் கோஸ்ட்") },
		{ TEXT("Woodlands South"), TEXT("兀兰南"), TEXT(""), TEXT("உட்லண்ட்ஸ் சவுத்") },
		{ TEXT("Springleaf"), TEXT("春叶"), TEXT(""), TEXT("ஸ்பிரிங்லீஃப்") },
		{ TEXT("Lentor"), TEXT("伦多"), TEXT(""), TEXT("லெண்டோர்") },
		{ TEXT("Mayflower"), TEXT("梅花"), TEXT(""), TEXT("மேஃபிளவர்") },
		{ TEXT("Bright Hill"), TEXT("光明山"), TEXT(""), TEXT("பிரைட் ஹில்") },
		{ TEXT("Upper Thomson"), TEXT("汤申路上段"), TEXT(""), TEXT("அப்பர் தாம்சன்") },
		{ TEXT("Napier"), TEXT("纳皮尔"), TEXT(""), TEXT("நேப்பியர்") },
		{ TEXT("Orchard Boulevard"), TEXT("乌节林荫道"), TEXT(""), TEXT("ஆர்ச்சர்ட் பவுல்வார்டு") },
		{ TEXT("Great World"), TEXT("大世界"), TEXT(""), TEXT("கிரேட் வேர்ல்ட்") },
		{ TEXT("Havelock"), TEXT("夏弗洛"), TEXT(""), TEXT("ஹேவ்லாக்") },
		{ TEXT("Maxwell"), TEXT("麦士威"), TEXT(""), TEXT("மேக்ஸ்வெல்") },
		{ TEXT("Shenton Way"), TEXT("珊顿道"), TEXT(""), TEXT("ஷென்டன் வே") },
		{ TEXT("Tanjong Rhu"), TEXT("丹戎禺"), TEXT(""), TEXT("தஞ்சோங் ரூ") },
		{ TEXT("Katong Park"), TEXT("加东公园"), TEXT(""), TEXT("கட்டோங் பார்க்") },
		{ TEXT("Tanjong Katong"), TEXT("丹戎加东"), TEXT(""), TEXT("தஞ்சோங் கட்டோங்") },
		{ TEXT("Siglap"), TEXT("实乞纳"), TEXT(""), TEXT("சிக்லாப்") },
		{ TEXT("Bedok South"), TEXT("勿洛南"), TEXT(""), TEXT("பெடோக் சவுத்") },
		// Places
		{ TEXT("Marina Bay Sands"), TEXT("滨海湾金沙"), TEXT(""), TEXT("மரினா பே சேண்ட்ஸ்") },
		{ TEXT("Merlion"), TEXT("鱼尾狮"), TEXT(""), TEXT("மெர்லயன்") },
		{ TEXT("Sentosa"), TEXT("圣淘沙"), TEXT(""), TEXT("செந்தோசா") },
		{ TEXT("Universal Studios Singapore"), TEXT("新加坡环球影城"), TEXT(""), TEXT("யுனிவர்சல் ஸ்டுடியோஸ் சிங்கப்பூர்") },
		{ TEXT("S.E.A. Aquarium"), TEXT("海洋生物园"), TEXT("Akuarium S.E.A."), TEXT("எஸ்.இ.ஏ. மீன்காட்சியகம்") },
		{ TEXT("Mount Faber"), TEXT("花拔山"), TEXT("Bukit Faber"), TEXT("மவுண்ட் ஃபேபர்") },
		{ TEXT("Singapore Zoo"), TEXT("新加坡动物园"), TEXT("Zoo Singapura"), TEXT("சிங்கப்பூர் மிருகக்காட்சிசாலை") },
		{ TEXT("Singapore Botanic Gardens"), TEXT("新加坡植物园"), TEXT("Taman Botani Singapura"), TEXT("சிங்கப்பூர் தாவரவியல் பூங்கா") },
		{ TEXT("Singapore Flyer"), TEXT("新加坡摩天观景轮"), TEXT(""), TEXT("சிங்கப்பூர் ஃபிளையர்") },
		{ TEXT("Esplanade Theatres"), TEXT("滨海艺术中心"), TEXT(""), TEXT("எஸ்பிளனேட் அரங்குகள்") },
		{ TEXT("Boat Quay"), TEXT("驳船码头"), TEXT(""), TEXT("போட் கீ") },
		{ TEXT("Chinatown Heritage Centre"), TEXT("牛车水遗产中心"), TEXT(""), TEXT("சைனாடவுன் பாரம்பரிய நிலையம்") },
		{ TEXT("Buddha Tooth Relic Temple"), TEXT("佛牙寺龙华院"), TEXT(""), TEXT("புத்தர் பல் நினைவுச்சின்னக் கோயில்") },
		{ TEXT("Sri Mariamman Temple"), TEXT("马里安曼兴都庙"), TEXT(""), TEXT("ஸ்ரீ மாரியம்மன் கோயில்") },
		{ TEXT("Fort Canning Park"), TEXT("福康宁公园"), TEXT("Taman Fort Canning"), TEXT("ஃபோர்ட் கேனிங் பூங்கா") },
		{ TEXT("Marina Barrage"), TEXT("滨海堤坝"), TEXT("Tambak Marina"), TEXT("மரினா தடுப்பணை") },
		{ TEXT("East Coast Park"), TEXT("东海岸公园"), TEXT("Taman Pantai Timur"), TEXT("ஈஸ்ட் கோஸ்ட் பூங்கா") },
		{ TEXT("Singapore Expo"), TEXT("新加坡博览中心"), TEXT(""), TEXT("சிங்கப்பூர் எக்ஸ்போ") },
		{ TEXT("Bukit Timah Nature Reserve"), TEXT("武吉知马自然保护区"), TEXT("Rizab Alam Bukit Timah"), TEXT("புக்கிட் தீமா இயற்கைக் காப்பகம்") },
		{ TEXT("Kent Ridge Park"), TEXT("肯特岗公园"), TEXT(""), TEXT("கெண்ட் ரிட்ஜ் பூங்கா") },
		{ TEXT("Pasir Ris Park"), TEXT("巴西立公园"), TEXT(""), TEXT("பாசிர் ரிஸ் பூங்கா") },
		{ TEXT("Bishan-AMK Park"), TEXT("碧山-宏茂桥公园"), TEXT(""), TEXT("பீஷான்-ஆங் மோ கியோ பூங்கா") },
		{ TEXT("Science Centre Singapore"), TEXT("新加坡科学馆"), TEXT("Pusat Sains Singapura"), TEXT("சிங்கப்பூர் அறிவியல் மையம்") },
		{ TEXT("Woodlands Checkpoint"), TEXT("兀兰关卡"), TEXT("Pusat Pemeriksaan Woodlands"), TEXT("உட்லண்ட்ஸ் சோதனைச்சாவடி") },
		{ TEXT("ION Orchard"), TEXT("ION乌节"), TEXT(""), TEXT("ஐயான் ஆர்ச்சர்ட்") },
		{ TEXT("Orchard Road"), TEXT("乌节路"), TEXT("Jalan Orchard"), TEXT("ஆர்ச்சர்ட் ரோடு") },
		{ TEXT("VivoCity"), TEXT("怡丰城"), TEXT(""), TEXT("விவோசிட்டி") },
		{ TEXT("Suntec City"), TEXT("新达城"), TEXT(""), TEXT("சன்டெக் சிட்டி") },
		{ TEXT("Marina Square"), TEXT("滨海广场"), TEXT(""), TEXT("மரினா ஸ்கொயர்") },
		{ TEXT("Bugis Junction"), TEXT("武吉士 Junction"), TEXT(""), TEXT("புகிஸ் ஜங்ஷன்") },
		{ TEXT("Mustafa Centre"), TEXT("慕斯达法中心"), TEXT(""), TEXT("முஸ்தபா சென்டர்") },
		{ TEXT("Chinatown Point"), TEXT("牛车水坊"), TEXT(""), TEXT("சைனாடவுன் பாயிண்ட்") },
		{ TEXT("Plaza Singapura"), TEXT("新加坡广场"), TEXT(""), TEXT("பிளாசா சிங்கப்பூரா") },
		{ TEXT("Tampines Mall"), TEXT("淡滨尼广场"), TEXT(""), TEXT("தெம்பனீஸ் மால்") },
		{ TEXT("Jurong Point"), TEXT("裕廊坊"), TEXT(""), TEXT("ஜூரோங் பாயிண்ட்") },
		{ TEXT("JEM and Westgate"), TEXT("星宇汇与西城"), TEXT(""), TEXT("ஜெம் & வெஸ்ட்கேட்") },
		{ TEXT("Causeway Point"), TEXT("兀兰海峡坊"), TEXT(""), TEXT("காஸ்வே பாயிண்ட்") },
		{ TEXT("Waterway Point"), TEXT("榜鹅水道坊"), TEXT(""), TEXT("வாட்டர்வே பாயிண்ட்") },
		{ TEXT("NEX Mall"), TEXT("NEX购物中心"), TEXT(""), TEXT("நெக்ஸ் மால்") },
		{ TEXT("Paya Lebar Quarter"), TEXT("巴耶利巴坊"), TEXT(""), TEXT("பாயா லேபார் குவார்ட்டர்") },
		{ TEXT("AMK Hub"), TEXT("宏茂桥中心"), TEXT(""), TEXT("ஆங் மோ கியோ ஹப்") },
		{ TEXT("Junction 8"), TEXT("碧山8号广场"), TEXT(""), TEXT("ஜங்ஷன் 8") },
		{ TEXT("Singapore General Hospital"), TEXT("新加坡中央医院"), TEXT("Hospital Besar Singapura"), TEXT("சிங்கப்பூர் பொது மருத்துவமனை") },
		{ TEXT("Tan Tock Seng Hospital"), TEXT("陈笃生医院"), TEXT("Hospital Tan Tock Seng"), TEXT("டான் டோக் செங் மருத்துவமனை") },
		{ TEXT("National University Hospital"), TEXT("国立大学医院"), TEXT("Hospital Universiti Nasional"), TEXT("தேசிய பல்கலைக்கழக மருத்துவமனை") },
		{ TEXT("Changi General Hospital"), TEXT("樟宜综合医院"), TEXT("Hospital Besar Changi"), TEXT("சாங்கி பொது மருத்துவமனை") },
		{ TEXT("KK Women's and Children's Hospital"), TEXT("竹脚妇幼医院"), TEXT("Hospital Wanita dan Kanak-kanak KK"), TEXT("கே.கே. மகளிர் குழந்தைகள் மருத்துவமனை") },
		{ TEXT("Khoo Teck Puat Hospital"), TEXT("邱德拔医院"), TEXT("Hospital Khoo Teck Puat"), TEXT("கூ டெக் புவாட் மருத்துவமனை") },
		{ TEXT("Mount Elizabeth Hospital"), TEXT("伊丽莎白医院"), TEXT("Hospital Mount Elizabeth"), TEXT("மவுண்ட் எலிசபெத் மருத்துவமனை") },
		{ TEXT("National University of Singapore"), TEXT("新加坡国立大学"), TEXT("Universiti Nasional Singapura"), TEXT("சிங்கப்பூர் தேசியப் பல்கலைக்கழகம்") },
		{ TEXT("Nanyang Technological University"), TEXT("南洋理工大学"), TEXT("Universiti Teknologi Nanyang"), TEXT("நன்யாங் தொழில்நுட்பப் பல்கலைக்கழகம்") },
		{ TEXT("Singapore Management University"), TEXT("新加坡管理大学"), TEXT("Universiti Pengurusan Singapura"), TEXT("சிங்கப்பூர் மேலாண்மைப் பல்கலைக்கழகம்") },
		{ TEXT("Jewel Changi Airport"), TEXT("星耀樟宜"), TEXT(""), TEXT("ஜுவல் சாங்கி விமான நிலையம்") },
		{ TEXT("Lau Pa Sat"), TEXT("老巴刹"), TEXT(""), TEXT("லாவ் பா சாட்") },
		{ TEXT("Maxwell Food Centre"), TEXT("麦士威熟食中心"), TEXT("Pusat Makanan Maxwell"), TEXT("மேக்ஸ்வெல் உணவங்காடி நிலையம்") },
		{ TEXT("Newton Food Centre"), TEXT("纽顿熟食中心"), TEXT("Pusat Makanan Newton"), TEXT("நியூட்டன் உணவங்காடி நிலையம்") },
		{ TEXT("Tekka Centre"), TEXT("竹脚中心"), TEXT("Pusat Tekka"), TEXT("தேக்கா நிலையம்") },
		{ TEXT("East Coast Lagoon Food Village"), TEXT("东海岸湖畔美食村"), TEXT("Kampung Makanan Lagun Pantai Timur"), TEXT("ஈஸ்ட் கோஸ்ட் லகூன் உணவுக் கிராமம்") },
		{ TEXT("Gluttons Bay"), TEXT("老饕海湾"), TEXT(""), TEXT("குளுட்டன்ஸ் பே") },
	};

	const FNameRow* FindName(const FString& English)
	{
		static TMap<FString, const FNameRow*> Index;
		if (Index.IsEmpty())
		{
			for (const FNameRow& Row : GNames)
			{
				Index.Add(Row.English, &Row);
			}
		}
		const FNameRow* const* Found = Index.Find(English);
		return Found ? *Found : nullptr;
	}
}

const TArray<FKioskPlace>& FKioskPlaces::All()
{
	static const TArray<FKioskPlace> Data = BuildPlaces();
	return Data;
}

int32 FKioskPlaces::FindPlace(const FString& Name)
{
	const TArray<FKioskPlace>& Places = All();
	for (int32 Index = 0; Index < Places.Num(); ++Index)
	{
		if (Places[Index].Name == Name)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

const TArray<FKioskAlias>& FKioskPlaces::Aliases()
{
	static const TArray<FKioskAlias> Data(GAliasRows, UE_ARRAY_COUNT(GAliasRows));
	return Data;
}

FText FKioskPlaces::CategoryLabel(EKioskPlaceCategory Category)
{
	switch (Category)
	{
	case C::Attraction: return KT("Attraction");
	case C::Shopping: return KT("Shopping");
	case C::Hospital: return KT("Hospital");
	case C::Airport: return KT("Airport");
	case C::Education: return KT("Education");
	case C::Food: return KT("Food");
	default: return KT("Area");
	}
}

EKioskIcon FKioskPlaces::CategoryIcon(EKioskPlaceCategory Category)
{
	switch (Category)
	{
	case C::Attraction: return EKioskIcon::Star;
	case C::Shopping: return EKioskIcon::Bag;
	case C::Hospital: return EKioskIcon::Cross;
	case C::Airport: return EKioskIcon::Plane;
	case C::Education: return EKioskIcon::Cap;
	case C::Food: return EKioskIcon::Fork;
	default: return EKioskIcon::Pin;
	}
}

FString FKioskPlaces::LocalNameIn(const FString& EnglishName, int32 LanguageIndex)
{
	const FNameRow* Row = FindName(EnglishName);
	if (!Row)
	{
		return FString();
	}
	const TCHAR* Local = LanguageIndex == 0 ? Row->Chinese : (LanguageIndex == 1 ? Row->Malay : Row->Tamil);
	return (Local[0] != TEXT('\0') && EnglishName != Local) ? FString(Local) : FString();
}

FString FKioskPlaces::LocalName(const FString& EnglishName)
{
	switch (FKioskLoc::GetLanguage())
	{
	case EKioskLanguage::Chinese: return LocalNameIn(EnglishName, 0);
	case EKioskLanguage::Malay: return LocalNameIn(EnglishName, 1);
	case EKioskLanguage::Tamil: return LocalNameIn(EnglishName, 2);
	default: return FString();
	}
}

FText FKioskPlaces::Note(const FKioskPlace& Place)
{
	if (Place.Name == TEXT("Singapore Zoo"))
	{
		return KT("shuttle bus from Khatib");
	}
	return FText::GetEmpty();
}

FText FKioskPlaces::NearLine(const FKioskPlace& Place)
{
	const FText Near = FText::Format(KT("near {0}"),
		StationLabel(FKioskData::Stations()[Place.Station].Name));
	const FText Extra = Note(Place);
	return Extra.IsEmpty() ? Near : FText::Format(KT("{0} · {1}"), Near, Extra);
}

FText FKioskPlaces::StationLabel(const FString& EnglishName)
{
	const FString Local = LocalName(EnglishName);
	return FText::FromString(Local.IsEmpty() ? EnglishName : FString::Printf(TEXT("%s (%s)"), *EnglishName, *Local));
}

FText FKioskPlaces::Blurb(int32 StationIndex)
{
	const TArray<FKioskStation>& Stations = FKioskData::Stations();
	if (!Stations.IsValidIndex(StationIndex))
	{
		return FText::GetEmpty();
	}
	const FString& Name = Stations[StationIndex].Name;
	if (Name == TEXT("HarbourFront")) return KT("VivoCity · Sentosa");
	if (Name == TEXT("Bayfront")) return KT("Gardens by the Bay · Marina Bay Sands");
	if (Name == TEXT("Changi Airport")) return KT("Flights · Jewel");
	if (Name == TEXT("Orchard")) return KT("Shopping · ION Orchard");
	if (Name == TEXT("Chinatown")) return KT("Heritage · food · Chinatown Point");
	return FText::GetEmpty();
}


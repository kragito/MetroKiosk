#include "KioskLoc.h"

namespace
{
	EKioskLanguage GLanguage = EKioskLanguage::English;

	struct FTranslation
	{
		const TCHAR* English;
		const TCHAR* Chinese;
		const TCHAR* Malay;
		const TCHAR* Tamil;
	};

	// Chrome and main headings only; anything missing falls back to English.
	const FTranslation GTable[] = {
		{ TEXT("Tap card"), TEXT("刷卡"), TEXT("Tap kad"), TEXT("அட்டையைத் தட்டு") },
		{ TEXT("What to do?"), TEXT("办理什么？"), TEXT("Nak buat apa?"), TEXT("என்ன செய்ய?") },
		{ TEXT("Top-up"), TEXT("增值"), TEXT("Tambah nilai"), TEXT("மீள்நிரப்பு") },
		{ TEXT("Concession Pass"), TEXT("优惠月票"), TEXT("Pas Konsesi"), TEXT("கட்டணக் கழிவு பாஸ்") },
		{ TEXT("All lines"), TEXT("所有线路"), TEXT("Semua laluan"), TEXT("அனைத்து வழித்தடங்கள்") },
		{ TEXT("Done"), TEXT("完成"), TEXT("Selesai"), TEXT("முடிந்தது") },
		{ TEXT("Use any method below. The lit slot on the kiosk shows you where."), TEXT("请任选以下方式。机器上亮灯的插槽会指示位置。"), TEXT("Gunakan mana-mana kaedah di bawah. Slot yang menyala menunjukkan lokasinya."), TEXT("கீழுள்ள எந்த முறையையும் பயன்படுத்தலாம். ஒளிரும் துளை இடத்தைக் காட்டும்.") },
		{ TEXT("Prices are for one adult. Add children or seniors on the Review step."), TEXT("价格为一位成人。儿童或乐龄人士请在核对步骤添加。"), TEXT("Harga untuk seorang dewasa. Tambah kanak-kanak atau warga emas pada langkah Semak."), TEXT("விலைகள் ஒரு வயது வந்தவருக்கு. குழந்தைகள் அல்லது மூத்தோரை சரிபார்ப்புப் படியில் சேர்க்கவும்.") },
		{ TEXT("Best for 3+ rides"), TEXT("3次以上乘车最划算"), TEXT("Terbaik untuk 3+ perjalanan"), TEXT("3+ பயணங்களுக்குச் சிறந்தது") },
		{ TEXT("Hold it flat on the reader until you hear a beep."), TEXT("平放在读卡器上，直到听到哔声。"), TEXT("Letakkan rata pada pembaca sehingga anda mendengar bunyi bip."), TEXT("பீப் ஒலி கேட்கும் வரை ரீடரில் தட்டையாக வைக்கவும்.") },
		{ TEXT("Bills up to $20 and coins. Change drops into the tray."), TEXT("接受20元以下纸币和硬币。找零会掉入托盘。"), TEXT("Wang kertas sehingga $20 dan syiling. Baki jatuh ke dalam dulang."), TEXT("$20 வரை நோட்டுகள் மற்றும் நாணயங்கள். மீதம் தட்டில் விழும்.") },
		{ TEXT("Pay with chip card/NETs"), TEXT("芯片卡/NETS付款"), TEXT("Bayar dengan kad cip/NETS"), TEXT("சிப் அட்டை/NETS மூலம் செலுத்து") },
		{ TEXT("Bank card"), TEXT("银行卡"), TEXT("Kad bank"), TEXT("வங்கி அட்டை") },
		{ TEXT("Metro card"), TEXT("地铁卡"), TEXT("Kad Metro"), TEXT("மெட்ரோ அட்டை") },
		{ TEXT("Chip first"), TEXT("芯片朝前"), TEXT("Cip dahulu"), TEXT("சிப் முதலில்") },
		{ TEXT("facing up"), TEXT("朝上"), TEXT("menghadap atas"), TEXT("மேல்நோக்கி") },
		{ TEXT("You are here"), TEXT("您在此处"), TEXT("Anda di sini"), TEXT("நீங்கள் இங்கே") },
		{ TEXT("Back to amounts"), TEXT("返回金额"), TEXT("Kembali ke jumlah"), TEXT("தொகைகளுக்குத் திரும்பு") },
		{ TEXT("Tap a station · drag to move · +/- to zoom"), TEXT("点按车站 · 拖动移动 · +/- 缩放"), TEXT("Ketik stesen · seret untuk alih · +/- untuk zum"), TEXT("நிலையத்தைத் தட்டவும் · இழுத்து நகர்த்தவும் · +/- பெரிதாக்கு") },
		{ TEXT("A keyboard will popup. The list narrows with every letter you type."), TEXT("将弹出键盘。每输入一个字母，列表就会缩小。"), TEXT("Papan kekunci akan muncul. Senarai menyempit setiap huruf yang ditaip."), TEXT("விசைப்பலகை தோன்றும். ஒவ்வொரு எழுத்திலும் பட்டியல் சுருங்கும்.") },
		{ TEXT("Metro card reader · below the screen"), TEXT("地铁卡读卡器 · 屏幕下方"), TEXT("Pembaca kad Metro · di bawah skrin"), TEXT("மெட்ரோ அட்டை ரீடர் · திரைக்குக் கீழே") },
		{ TEXT("Card reader · top right of the screen"), TEXT("读卡器 · 屏幕右上方"), TEXT("Pembaca kad · kanan atas skrin"), TEXT("அட்டை ரீடர் · திரையின் மேல் வலது") },
		{ TEXT("Back"), TEXT("返回"), TEXT("Kembali"), TEXT("பின்செல்") },
		{ TEXT("Start over"), TEXT("重新开始"), TEXT("Mula semula"), TEXT("மீண்டும் தொடங்கு") },
		{ TEXT("Help · talk to staff"), TEXT("帮助 · 联系工作人员"), TEXT("Bantuan · hubungi kakitangan"), TEXT("உதவி · ஊழியருடன் பேசுங்கள்") },
		{ TEXT("You are at"), TEXT("您现在在"), TEXT("Anda berada di"), TEXT("நீங்கள் இருக்கும் இடம்") },
		{ TEXT("City Hall"), TEXT("政府大厦"), TEXT("City Hall"), TEXT("சிட்டி ஹால்") },
		{ TEXT("NO CARD"), TEXT("没有卡"), TEXT("TIADA KAD"), TEXT("அட்டை இல்லை") },
		{ TEXT("HAVE A CARD"), TEXT("持有卡"), TEXT("ADA KAD"), TEXT("அட்டை உள்ளது") },
		{ TEXT("HELP & SETTINGS"), TEXT("帮助与设置"), TEXT("BANTUAN & TETAPAN"), TEXT("உதவி & அமைப்புகள்") },
		{ TEXT("One-time ticket"), TEXT("单次车票"), TEXT("Tiket sekali guna"), TEXT("ஒருமுறை பயணச்சீட்டு") },
		{ TEXT("Printed ticket for one trip. Choose one-way, round trip or a day pass."), TEXT("单次行程的纸质车票。可选单程、往返或日票。"), TEXT("Tiket bercetak untuk satu perjalanan. Pilih sehala, pergi balik atau pas harian."), TEXT("ஒரு பயணத்திற்கான அச்சிட்ட பயணச்சீட்டு. ஒருவழி, சுற்றுப் பயணம் அல்லது நாள் பாஸ் தேர்வு செய்யவும்.") },
		{ TEXT("Buy a ticket"), TEXT("购买车票"), TEXT("Beli tiket"), TEXT("பயணச்சீட்டு வாங்கவும்") },
		{ TEXT("Top up"), TEXT("增值"), TEXT("Tambah nilai"), TEXT("மீள்நிரப்பு") },
		{ TEXT("Add value and check your balance"), TEXT("增值并查询余额"), TEXT("Tambah nilai dan semak baki"), TEXT("மதிப்பைச் சேர்த்து இருப்பைப் பார்க்கவும்") },
		{ TEXT("Concession"), TEXT("优惠"), TEXT("Konsesi"), TEXT("கட்டணக் கழிவு") },
		{ TEXT("Student, senior or disability card: buy or renew a pass"), TEXT("学生卡、乐龄卡或残障卡：购买或续订月票"), TEXT("Kad pelajar, warga emas atau OKU: beli atau perbaharui pas"), TEXT("மாணவர், மூத்தோர் அல்லது ஊனமுற்றோர் அட்டை: பாஸ் வாங்கவும் அல்லது புதுப்பிக்கவும்") },
		{ TEXT("Language"), TEXT("语言"), TEXT("Bahasa"), TEXT("மொழி") },
		{ TEXT("Call for assistance"), TEXT("呼叫协助"), TEXT("Panggil bantuan"), TEXT("உதவிக்கு அழைக்கவும்") },
		{ TEXT("Video call with station staff"), TEXT("与车站工作人员视频通话"), TEXT("Panggilan video dengan kakitangan stesen"), TEXT("நிலைய ஊழியருடன் வீடியோ அழைப்பு") },
		{ TEXT("Staff online"), TEXT("工作人员在线"), TEXT("Kakitangan dalam talian"), TEXT("ஊழியர் இணையத்தில்") },
		{ TEXT("Or place your card on the reader to begin"), TEXT("或将您的卡放在读卡器上开始"), TEXT("Atau letakkan kad anda pada pembaca untuk bermula"), TEXT("அல்லது தொடங்க உங்கள் அட்டையை ரீடரில் வைக்கவும்") },
		{ TEXT("We'll show your balance and what you can do with it"), TEXT("我们将显示您的余额及可办理的业务"), TEXT("Kami akan tunjukkan baki anda dan apa yang boleh dilakukan"), TEXT("உங்கள் இருப்பையும் செய்யக்கூடியவற்றையும் காட்டுவோம்") },
		{ TEXT("Reader here"), TEXT("读卡器在此"), TEXT("Pembaca di sini"), TEXT("ரீடர் இங்கே") },
		{ TEXT("Choose a ticket"), TEXT("选择车票"), TEXT("Pilih tiket"), TEXT("பயணச்சீட்டைத் தேர்ந்தெடுக்கவும்") },
		{ TEXT("One-way"), TEXT("单程"), TEXT("Sehala"), TEXT("ஒருவழி") },
		{ TEXT("Round trip"), TEXT("往返"), TEXT("Pergi balik"), TEXT("சுற்றுப் பயணம்") },
		{ TEXT("Day pass"), TEXT("日票"), TEXT("Pas harian"), TEXT("நாள் பாஸ்") },
		{ TEXT("Choose"), TEXT("选择"), TEXT("Pilih"), TEXT("தேர்வு") },
		{ TEXT("Ticket"), TEXT("车票"), TEXT("Tiket"), TEXT("பயணச்சீட்டு") },
		{ TEXT("Destination"), TEXT("目的地"), TEXT("Destinasi"), TEXT("சேருமிடம்") },
		{ TEXT("Review"), TEXT("核对"), TEXT("Semak"), TEXT("சரிபார்") },
		{ TEXT("Pay"), TEXT("付款"), TEXT("Bayar"), TEXT("செலுத்து") },
		{ TEXT("Where are you going?"), TEXT("您要去哪里？"), TEXT("Anda hendak ke mana?"), TEXT("நீங்கள் எங்கே செல்கிறீர்கள்?") },
		{ TEXT("Popular from City Hall"), TEXT("政府大厦热门目的地"), TEXT("Popular dari City Hall"), TEXT("சிட்டி ஹாலிலிருந்து பிரபலமானவை") },
		{ TEXT("Select a location"), TEXT("请选择地点"), TEXT("Pilih lokasi"), TEXT("இடத்தைத் தேர்ந்தெடுக்கவும்") },
		{ TEXT("Somewhere else?"), TEXT("其他地方？"), TEXT("Tempat lain?"), TEXT("வேறு இடமா?") },
		{ TEXT("Search all stations"), TEXT("搜索所有车站"), TEXT("Cari semua stesen"), TEXT("அனைத்து நிலையங்களையும் தேடுங்கள்") },
		{ TEXT("Filter by line"), TEXT("按线路筛选"), TEXT("Tapis mengikut laluan"), TEXT("வழித்தடம் வாரியாக வடிகட்டு") },
		{ TEXT("Open metro map view"), TEXT("打开地铁地图"), TEXT("Buka paparan peta"), TEXT("வரைபடத்தைத் திற") },
		{ TEXT("Open station list view"), TEXT("打开车站列表"), TEXT("Buka senarai stesen"), TEXT("நிலையப் பட்டியலைத் திற") },
		{ TEXT("Confirm selected ticket"), TEXT("确认所选车票"), TEXT("Sahkan tiket dipilih"), TEXT("தேர்ந்த பயணச்சீட்டை உறுதிசெய்க") },
		{ TEXT("Check your trip"), TEXT("核对您的行程"), TEXT("Semak perjalanan anda"), TEXT("உங்கள் பயணத்தைச் சரிபார்க்கவும்") },
		{ TEXT("Who is riding?"), TEXT("谁乘车？"), TEXT("Siapa yang menaiki?"), TEXT("யார் பயணம் செய்கிறார்கள்?") },
		{ TEXT("Adult"), TEXT("成人"), TEXT("Dewasa"), TEXT("வயது வந்தோர்") },
		{ TEXT("Child (5–17)"), TEXT("儿童 (5–17)"), TEXT("Kanak-kanak (5–17)"), TEXT("குழந்தை (5–17)") },
		{ TEXT("Senior (65+)"), TEXT("乐龄人士 (65+)"), TEXT("Warga emas (65+)"), TEXT("மூத்தோர் (65+)") },
		{ TEXT("Go to payment"), TEXT("前往付款"), TEXT("Pergi ke pembayaran"), TEXT("பணம் செலுத்தச் செல்லவும்") },
		{ TEXT("Tap card or phone"), TEXT("刷卡或手机"), TEXT("Tap kad atau telefon"), TEXT("அட்டை அல்லது தொலைபேசியைத் தட்டவும்") },
		{ TEXT("Insert chip card"), TEXT("插入芯片卡"), TEXT("Masukkan kad cip"), TEXT("சிப் அட்டையைச் செருகவும்") },
		{ TEXT("Pay with cash"), TEXT("现金付款"), TEXT("Bayar dengan tunai"), TEXT("பணமாகச் செலுத்து") },
		{ TEXT("Pay with card"), TEXT("刷卡付款"), TEXT("Bayar dengan kad"), TEXT("அட்டையால் செலுத்து") },
		{ TEXT("Fastest"), TEXT("最快"), TEXT("Terpantas"), TEXT("வேகமானது") },
		{ TEXT("Hold your card or phone on the reader"), TEXT("请将卡或手机贴近读卡器"), TEXT("Letakkan kad atau telefon anda pada pembaca"), TEXT("உங்கள் அட்டை அல்லது தொலைபேசியை ரீடரில் வைக்கவும்") },
		{ TEXT("Insert your card, chip first"), TEXT("请插入卡片，芯片朝前"), TEXT("Masukkan kad anda, cip dahulu"), TEXT("உங்கள் அட்டையைச் சிப் முதலில் செருகவும்") },
		{ TEXT("Insert notes or coins"), TEXT("请投入纸币或硬币"), TEXT("Masukkan wang kertas atau syiling"), TEXT("நோட்டுகள் அல்லது நாணயங்களைச் செலுத்தவும்") },
		{ TEXT("Processing..."), TEXT("处理中..."), TEXT("Memproses..."), TEXT("செயலாக்கப்படுகிறது...") },
		{ TEXT("Payment approved"), TEXT("付款已批准"), TEXT("Pembayaran diluluskan"), TEXT("பணம் செலுத்தல் ஏற்கப்பட்டது") },
		{ TEXT("Want a receipt?"), TEXT("需要收据吗？"), TEXT("Mahu resit?"), TEXT("ரசீது வேண்டுமா?") },
		{ TEXT("Print receipt"), TEXT("打印收据"), TEXT("Cetak resit"), TEXT("ரசீதை அச்சிடு") },
		{ TEXT("Email receipt"), TEXT("电邮收据"), TEXT("E-mel resit"), TEXT("ரசீதை மின்னஞ்சலிடு") },
		{ TEXT("No receipt · I'm done"), TEXT("不需要收据 · 完成"), TEXT("Tiada resit · Selesai"), TEXT("ரசீது வேண்டாம் · முடிந்தது") },
		{ TEXT("+ Buy another ticket"), TEXT("+ 再买一张车票"), TEXT("+ Beli tiket lain"), TEXT("+ மற்றொரு பயணச்சீட்டு வாங்கு") },
		{ TEXT("Place your transit card on the reader"), TEXT("请将交通卡放在读卡器上"), TEXT("Letakkan kad transit anda pada pembaca"), TEXT("உங்கள் போக்குவரத்து அட்டையை ரீடரில் வைக்கவும்") },
		{ TEXT("Card found. What would you like to do?"), TEXT("已找到卡片。您想做什么？"), TEXT("Kad ditemui. Apa yang anda mahu lakukan?"), TEXT("அட்டை கிடைத்தது. என்ன செய்ய விரும்புகிறீர்கள்?") },
		{ TEXT("Quick top up"), TEXT("快速增值"), TEXT("Tambah nilai pantas"), TEXT("விரைவு மீள்நிரப்பு") },
		{ TEXT("Other card options"), TEXT("其他卡片选项"), TEXT("Pilihan kad lain"), TEXT("பிற அட்டை விருப்பங்கள்") },
		{ TEXT("Top up another amount"), TEXT("增值其他金额"), TEXT("Tambah jumlah lain"), TEXT("வேறு தொகையை நிரப்பு") },
		{ TEXT("Renew concession pass"), TEXT("续订优惠月票"), TEXT("Perbaharui pas konsesi"), TEXT("கட்டணக் கழிவு பாஸைப் புதுப்பி") },
		{ TEXT("Buy a ticket instead"), TEXT("改为购买车票"), TEXT("Beli tiket sebaliknya"), TEXT("அதற்குப் பதிலாக பயணச்சீட்டு வாங்கு") },
		{ TEXT("How much would you like to add?"), TEXT("您想增值多少？"), TEXT("Berapa banyak yang anda mahu tambah?"), TEXT("எவ்வளவு சேர்க்க விரும்புகிறீர்கள்?") },
		{ TEXT("Continue"), TEXT("继续"), TEXT("Teruskan"), TEXT("தொடரவும்") },
		{ TEXT("More"), TEXT("更多"), TEXT("Lagi"), TEXT("மேலும்") },
		{ TEXT("Choose a concession pass"), TEXT("选择优惠月票"), TEXT("Pilih pas konsesi"), TEXT("கட்டணக் கழிவு பாஸைத் தேர்ந்தெடுக்கவும்") },
		{ TEXT("Continue · check dates"), TEXT("继续 · 核对日期"), TEXT("Teruskan · semak tarikh"), TEXT("தொடரவும் · தேதிகளைச் சரிபார்") },
		{ TEXT("Check your pass dates"), TEXT("核对月票日期"), TEXT("Semak tarikh pas anda"), TEXT("உங்கள் பாஸ் தேதிகளைச் சரிபார்க்கவும்") },
		{ TEXT("Don't forget your card"), TEXT("请勿忘记您的卡"), TEXT("Jangan lupa kad anda"), TEXT("உங்கள் அட்டையை மறக்காதீர்கள்") },
		{ TEXT("Help · In call with staff"), TEXT("帮助 · 正在与工作人员通话"), TEXT("Bantuan · Dalam panggilan dengan kakitangan"), TEXT("உதவி · ஊழியருடன் அழைப்பில்") },
		{ TEXT("Back to my purchase"), TEXT("返回我的购买"), TEXT("Kembali ke pembelian saya"), TEXT("என் வாங்குதலுக்குத் திரும்பு") },
		{ TEXT("End call"), TEXT("结束通话"), TEXT("Tamatkan panggilan"), TEXT("அழைப்பை முடி") },
		{ TEXT("Back to call"), TEXT("返回通话"), TEXT("Kembali ke panggilan"), TEXT("அழைப்புக்குத் திரும்பு") },
		{ TEXT("Your purchase is paused."), TEXT("您的购买已暂停。"), TEXT("Pembelian anda dijeda."), TEXT("உங்கள் வாங்குதல் இடைநிறுத்தப்பட்டது.") },
		{ TEXT("Mute microphone"), TEXT("静音麦克风"), TEXT("Senyapkan mikrofon"), TEXT("ஒலிவாங்கியை முடக்கு") },
		{ TEXT("Ask staff to come here"), TEXT("请工作人员过来"), TEXT("Minta kakitangan datang ke sini"), TEXT("ஊழியரை இங்கே வரச்சொல்லுங்கள்") },
		{ TEXT("Captions are on"), TEXT("字幕已开启"), TEXT("Sari kata dihidupkan"), TEXT("வசனங்கள் இயக்கத்தில்") },
	};
}

void FKioskLoc::SetLanguage(EKioskLanguage InLanguage)
{
	GLanguage = InLanguage;
}

EKioskLanguage FKioskLoc::GetLanguage()
{
	return GLanguage;
}

FText FKioskLoc::Tr(const FText& English)
{
	if (GLanguage == EKioskLanguage::English)
	{
		return English;
	}

	static TMap<FString, const FTranslation*> Index;
	if (Index.IsEmpty())
	{
		for (const FTranslation& Entry : GTable)
		{
			Index.Add(Entry.English, &Entry);
		}
	}

	if (const FTranslation* const* Found = Index.Find(English.ToString()))
	{
		const FTranslation& Entry = **Found;
		switch (GLanguage)
		{
		case EKioskLanguage::Chinese: return FText::FromString(Entry.Chinese);
		case EKioskLanguage::Malay: return FText::FromString(Entry.Malay);
		case EKioskLanguage::Tamil: return FText::FromString(Entry.Tamil);
		default: break;
		}
	}
	return English;
}

FText FKioskLoc::LanguageName(EKioskLanguage InLanguage)
{
	switch (InLanguage)
	{
	case EKioskLanguage::Chinese: return FText::FromString(TEXT("中文"));
	case EKioskLanguage::Malay: return FText::FromString(TEXT("Melayu"));
	case EKioskLanguage::Tamil: return FText::FromString(TEXT("தமிழ்"));
	default: return NSLOCTEXT("MetroKiosk", "LangEnglish", "English");
	}
}

#include "KioskData.h"
#include "KioskLoc.h"

int32 FKioskStation::FirstLine() const
{
	for (int32 Index = 0; Index < FKioskData::NumLines; ++Index)
	{
		if (OnLine(Index))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

const FKioskLineInfo& FKioskData::Line(int32 LineIndex)
{
	static const FKioskLineInfo Lines[NumLines] = {
		{ TEXT("NS"), TEXT("North–South"), TEXT("Red") },
		{ TEXT("EW"), TEXT("East–West"), TEXT("Green") },
		{ TEXT("NE"), TEXT("North East"), TEXT("Purple") },
		{ TEXT("CC"), TEXT("Circle"), TEXT("Orange") },
		{ TEXT("DT"), TEXT("Downtown"), TEXT("Blue") },
		{ TEXT("TE"), TEXT("Thomson"), TEXT("Brown") },
	};
	return Lines[FMath::Clamp(LineIndex, 0, NumLines - 1)];
}

namespace
{
	enum : uint8 { NS = 1, EW = 2, NE = 4, CC = 8, DT = 16, TE = 32 };

	struct FStationRow
	{
		const TCHAR* Name;
		uint8 Lines;
		float X; // schematic map space (the system map's layout, scaled), about 40..890 x 20..580
		float Y;
	};

	/** One point on a drawn line: a station (named; its position comes from the station table) or a bend (X, Y). */
	struct FShapePoint
	{
		const TCHAR* Station;
		float X;
		float Y;
	};

	struct FShapeRow
	{
		int32 Line;
		const FShapePoint* Points;
		int32 Num;
	};

	// Stations follow the LTA system map: positions are taken from it and scaled into schematic units.
	// Left out: LRT lines, interchange-only codes (BP1, STC, PTC) and anything under construction.
	const FStationRow GRows[] = {
		{ TEXT("Tuas Link"), EW, 41.8f, 223.7f },
		{ TEXT("Tuas West Road"), EW, 42.4f, 248.1f },
		{ TEXT("Tuas Crescent"), EW, 43.0f, 273.1f },
		{ TEXT("Gul Circle"), EW, 43.0f, 298.2f },
		{ TEXT("Joo Koon"), EW, 43.7f, 322.6f },
		{ TEXT("Pioneer"), EW, 44.3f, 348.2f },
		{ TEXT("Boon Lay"), EW, 56.5f, 370.8f },
		{ TEXT("Lakeside"), EW, 83.9f, 386.6f },
		{ TEXT("Chinese Garden"), EW, 116.9f, 387.8f },
		{ TEXT("Clementi"), EW, 201.7f, 387.8f },
		{ TEXT("Dover"), EW, 235.8f, 387.8f },
		{ TEXT("Commonwealth"), EW, 299.9f, 406.7f },
		{ TEXT("Queenstown"), EW, 316.3f, 423.2f },
		{ TEXT("Redhill"), EW, 337.7f, 442.7f },
		{ TEXT("Tiong Bahru"), EW, 357.8f, 463.5f },
		{ TEXT("Tanjong Pagar"), EW, 423.1f, 525.7f },
		{ TEXT("Lavender"), EW, 576.2f, 414.1f },
		{ TEXT("Kallang"), EW, 596.9f, 393.9f },
		{ TEXT("Aljunied"), EW, 616.4f, 373.8f },
		{ TEXT("Eunos"), EW, 669.5f, 320.1f },
		{ TEXT("Kembangan"), EW, 696.4f, 293.3f },
		{ TEXT("Bedok"), EW, 740.3f, 268.9f },
		{ TEXT("Tanah Merah"), EW, 798.2f, 268.9f },
		{ TEXT("Simei"), EW, 803.1f, 252.4f },
		{ TEXT("Pasir Ris"), EW, 803.1f, 201.2f },
		{ TEXT("Changi Airport"), EW, 889.1f, 302.4f },
		{ TEXT("Jurong East"), NS | EW, 157.1f, 387.8f },
		{ TEXT("Buona Vista"), EW | CC, 273.0f, 387.8f },
		{ TEXT("Outram Park"), EW | NE | TE, 395.0f, 484.2f },
		{ TEXT("City Hall"), NS | EW, 506.6f, 484.2f },
		{ TEXT("Raffles Place"), NS | EW, 506.6f, 512.9f },
		{ TEXT("Bugis"), EW | DT, 551.2f, 437.8f },
		{ TEXT("Paya Lebar"), EW | CC, 642.7f, 349.4f },
		{ TEXT("Tampines"), EW | DT, 803.7f, 234.1f },
		{ TEXT("Expo"), EW | DT, 844.0f, 302.4f },
		{ TEXT("Choa Chu Kang"), NS, 157.1f, 237.8f },
		{ TEXT("Bukit Gombak"), NS, 157.1f, 288.4f },
		{ TEXT("Bukit Batok"), NS, 157.1f, 338.4f },
		{ TEXT("Yew Tee"), NS, 157.1f, 176.8f },
		{ TEXT("Kranji"), NS, 157.1f, 118.2f },
		{ TEXT("Marsiling"), NS, 171.2f, 70.0f },
		{ TEXT("Admiralty"), NS, 312.7f, 51.7f },
		{ TEXT("Sembawang"), NS, 349.9f, 51.7f },
		{ TEXT("Canberra"), NS, 388.3f, 51.7f },
		{ TEXT("Yishun"), NS, 431.0f, 66.4f },
		{ TEXT("Khatib"), NS, 445.6f, 98.7f },
		{ TEXT("Yio Chu Kang"), NS, 445.6f, 126.8f },
		{ TEXT("Ang Mo Kio"), NS, 445.6f, 162.1f },
		{ TEXT("Braddell"), NS, 445.6f, 221.9f },
		{ TEXT("Toa Payoh"), NS, 445.6f, 250.6f },
		{ TEXT("Novena"), NS, 428.6f, 282.9f },
		{ TEXT("Somerset"), NS, 402.3f, 380.5f },
		{ TEXT("Marina South Pier"), NS, 538.4f, 581.2f },
		{ TEXT("Woodlands"), NS | TE, 260.2f, 51.7f },
		{ TEXT("Bishan"), NS | CC, 445.6f, 201.8f },
		{ TEXT("Newton"), NS | DT, 418.2f, 307.3f },
		{ TEXT("Orchard"), NS | TE, 371.8f, 350.6f },
		{ TEXT("Dhoby Ghaut"), NS | NE | CC, 449.9f, 417.1f },
		{ TEXT("Marina Bay"), NS | CC | TE, 517.0f, 556.2f },
		{ TEXT("Bras Basah"), CC, 510.3f, 443.9f },
		{ TEXT("Esplanade"), CC, 550.0f, 484.2f },
		{ TEXT("Nicoll Highway"), CC, 626.8f, 469.6f },
		{ TEXT("Stadium"), CC, 638.4f, 442.1f },
		{ TEXT("Mountbatten"), CC, 644.5f, 412.8f },
		{ TEXT("Dakota"), CC, 645.1f, 384.8f },
		{ TEXT("Tai Seng"), CC, 608.5f, 274.4f },
		{ TEXT("Bartley"), CC, 583.5f, 249.4f },
		{ TEXT("Lorong Chuan"), CC, 499.3f, 206.0f },
		{ TEXT("Marymount"), CC, 397.5f, 212.2f },
		{ TEXT("Farrer Road"), CC, 287.7f, 316.5f },
		{ TEXT("Holland Village"), CC, 276.7f, 351.2f },
		{ TEXT("one-north"), CC, 273.0f, 415.9f },
		{ TEXT("Kent Ridge"), CC, 279.7f, 440.3f },
		{ TEXT("Haw Par Villa"), CC, 287.7f, 464.1f },
		{ TEXT("Pasir Panjang"), CC, 302.3f, 491.5f },
		{ TEXT("Labrador Park"), CC, 321.2f, 515.3f },
		{ TEXT("Telok Blangah"), CC, 344.4f, 536.1f },
		{ TEXT("Keppel"), CC, 418.2f, 570.8f },
		{ TEXT("Cantonment"), CC, 453.0f, 575.7f },
		{ TEXT("Prince Edward Road"), CC, 489.0f, 572.7f },
		{ TEXT("Promenade"), CC | DT, 605.5f, 499.5f },
		{ TEXT("MacPherson"), CC | DT, 628.6f, 307.3f },
		{ TEXT("Serangoon"), CC | NE, 546.3f, 222.5f },
		{ TEXT("Caldecott"), CC | TE, 364.5f, 228.0f },
		{ TEXT("Botanic Gardens"), CC | DT, 310.2f, 279.2f },
		{ TEXT("HarbourFront"), NE | CC, 377.9f, 560.5f },
		{ TEXT("Bayfront"), CC | DT, 570.7f, 533.0f },
		{ TEXT("Bukit Panjang"), DT, 229.7f, 133.5f },
		{ TEXT("Cashew"), DT, 254.7f, 142.6f },
		{ TEXT("Hillview"), DT, 259.6f, 165.8f },
		{ TEXT("Hume"), DT, 259.6f, 188.4f },
		{ TEXT("Beauty World"), DT, 259.6f, 211.5f },
		{ TEXT("King Albert Park"), DT, 259.6f, 234.1f },
		{ TEXT("Sixth Avenue"), DT, 260.2f, 257.3f },
		{ TEXT("Tan Kah Kee"), DT, 271.8f, 272.5f },
		{ TEXT("Rochor"), DT, 500.6f, 386.0f },
		{ TEXT("Fort Canning"), DT, 419.4f, 429.3f },
		{ TEXT("Bencoolen"), DT, 517.0f, 420.8f },
		{ TEXT("Jalan Besar"), DT, 540.2f, 398.2f },
		{ TEXT("Bendemeer"), DT, 570.1f, 368.9f },
		{ TEXT("Geylang Bahru"), DT, 590.8f, 348.2f },
		{ TEXT("Mattar"), DT, 610.4f, 328.1f },
		{ TEXT("Ubi"), DT, 653.7f, 284.7f },
		{ TEXT("Kaki Bukit"), DT, 673.2f, 264.6f },
		{ TEXT("Bedok North"), DT, 694.5f, 245.7f },
		{ TEXT("Bedok Reservoir"), DT, 728.1f, 234.7f },
		{ TEXT("Tampines West"), DT, 763.5f, 234.7f },
		{ TEXT("Tampines East"), DT, 845.2f, 252.4f },
		{ TEXT("Upper Changi"), DT, 845.2f, 273.8f },
		{ TEXT("Xilin"), DT, 845.2f, 321.3f },
		{ TEXT("Downtown"), DT, 529.8f, 544.6f },
		{ TEXT("Telok Ayer"), DT, 478.6f, 514.1f },
		{ TEXT("Stevens"), DT | TE, 364.5f, 279.2f },
		{ TEXT("Little India"), NE | DT, 476.8f, 358.6f },
		{ TEXT("Chinatown"), NE | DT, 421.2f, 461.6f },
		{ TEXT("Sungei Bedok"), DT | TE, 845.2f, 340.9f },
		{ TEXT("Clarke Quay"), NE, 435.9f, 444.6f },
		{ TEXT("Farrer Park"), NE, 476.1f, 330.5f },
		{ TEXT("Boon Keng"), NE, 476.1f, 304.3f },
		{ TEXT("Potong Pasir"), NE, 492.0f, 276.2f },
		{ TEXT("Woodleigh"), NE, 517.0f, 250.6f },
		{ TEXT("Kovan"), NE, 567.0f, 201.2f },
		{ TEXT("Hougang"), NE, 585.3f, 182.3f },
		{ TEXT("Buangkok"), NE, 604.9f, 162.7f },
		{ TEXT("Sengkang"), NE, 617.1f, 143.8f },
		{ TEXT("Punggol"), NE, 663.4f, 96.9f },
		{ TEXT("Punggol Coast"), NE, 686.6f, 80.4f },
		{ TEXT("Woodlands North"), TE, 232.2f, 23.7f },
		{ TEXT("Woodlands South"), TE, 277.9f, 70.0f },
		{ TEXT("Springleaf"), TE, 298.0f, 91.4f },
		{ TEXT("Lentor"), TE, 318.2f, 112.7f },
		{ TEXT("Mayflower"), TE, 338.9f, 133.5f },
		{ TEXT("Bright Hill"), TE, 360.2f, 159.7f },
		{ TEXT("Upper Thomson"), TE, 364.5f, 192.6f },
		{ TEXT("Napier"), TE, 364.5f, 305.5f },
		{ TEXT("Orchard Boulevard"), TE, 364.5f, 328.1f },
		{ TEXT("Great World"), TE, 364.5f, 378.1f },
		{ TEXT("Havelock"), TE, 364.5f, 404.9f },
		{ TEXT("Maxwell"), TE, 431.6f, 501.9f },
		{ TEXT("Shenton Way"), TE, 476.8f, 545.2f },
		{ TEXT("Gardens by the Bay"), TE, 635.4f, 533.0f },
		{ TEXT("Tanjong Rhu"), TE, 676.2f, 492.8f },
		{ TEXT("Katong Park"), TE, 695.8f, 474.4f },
		{ TEXT("Tanjong Katong"), TE, 714.7f, 454.9f },
		{ TEXT("Marine Parade"), TE, 733.6f, 436.0f },
		{ TEXT("Marine Terrace"), TE, 753.1f, 416.5f },
		{ TEXT("Siglap"), TE, 772.0f, 397.0f },
		{ TEXT("Bayshore"), TE, 788.5f, 380.5f },
		{ TEXT("Bedok South"), TE, 806.8f, 362.2f },
	};

	const FShapePoint GNS0[] = {
		{ TEXT("Jurong East") }, { TEXT("Bukit Batok") }, { TEXT("Bukit Gombak") }, { TEXT("Choa Chu Kang") }, { TEXT("Yew Tee") }, { TEXT("Kranji") },
		{ nullptr, 157.1f, 108.5f }, { nullptr, 159.0f, 100.1f }, { nullptr, 162.1f, 89.5f }, { nullptr, 166.3f, 78.7f }, { TEXT("Marsiling") },
		{ nullptr, 180.5f, 60.8f }, { nullptr, 191.3f, 54.8f }, { nullptr, 201.8f, 52.4f }, { nullptr, 213.8f, 51.7f }, { TEXT("Woodlands") },
		{ TEXT("Admiralty") }, { TEXT("Sembawang") }, { TEXT("Canberra") }, { nullptr, 398.0f, 51.5f }, { nullptr, 407.1f, 51.4f },
		{ nullptr, 415.1f, 52.9f }, { nullptr, 423.8f, 58.8f }, { TEXT("Yishun") }, { nullptr, 437.6f, 73.7f }, { nullptr, 442.6f, 81.6f },
		{ nullptr, 444.8f, 89.7f }, { TEXT("Khatib") }, { TEXT("Yio Chu Kang") }, { TEXT("Ang Mo Kio") }, { TEXT("Bishan") }, { TEXT("Braddell") },
		{ TEXT("Toa Payoh") }, { nullptr, 445.6f, 262.2f }, { nullptr, 441.2f, 268.7f }, { nullptr, 434.5f, 275.6f }, { TEXT("Novena") },
		{ nullptr, 423.8f, 295.7f }, { TEXT("Newton") }, { nullptr, 395.0f, 317.7f }, { nullptr, 385.8f, 326.0f }, { nullptr, 378.6f, 335.4f },
		{ nullptr, 374.1f, 342.9f }, { TEXT("Orchard") }, { nullptr, 372.7f, 359.5f }, { nullptr, 376.7f, 367.7f }, { nullptr, 383.4f, 372.3f },
		{ nullptr, 392.6f, 375.9f }, { TEXT("Somerset") }, { nullptr, 411.1f, 386.2f }, { nullptr, 420.5f, 392.8f }, { nullptr, 430.3f, 400.2f },
		{ nullptr, 440.2f, 408.4f }, { TEXT("Dhoby Ghaut") }, { nullptr, 458.7f, 425.6f }, { nullptr, 468.3f, 435.1f }, { nullptr, 478.1f, 445.2f },
		{ nullptr, 487.4f, 455.6f }, { nullptr, 495.7f, 465.9f }, { nullptr, 502.3f, 475.5f }, { TEXT("City Hall") }, { nullptr, 508.4f, 494.3f },
		{ nullptr, 507.6f, 503.5f }, { TEXT("Raffles Place") }, { nullptr, 505.6f, 523.5f }, { nullptr, 504.5f, 534.3f }, { nullptr, 505.4f, 543.4f },
		{ nullptr, 510.6f, 550.1f }, { TEXT("Marina Bay") }, { nullptr, 529.2f, 573.9f }, { TEXT("Marina South Pier") },
	};
	const FShapePoint GEW0[] = {
		{ TEXT("Tuas Link") }, { TEXT("Tuas West Road") }, { TEXT("Tuas Crescent") }, { TEXT("Gul Circle") }, { TEXT("Joo Koon") }, { TEXT("Pioneer") },
		{ nullptr, 46.1f, 359.8f }, { TEXT("Boon Lay") }, { nullptr, 62.2f, 377.7f }, { nullptr, 68.7f, 383.6f }, { nullptr, 75.7f, 385.7f },
		{ TEXT("Lakeside") }, { TEXT("Chinese Garden") }, { TEXT("Jurong East") }, { TEXT("Clementi") }, { TEXT("Dover") }, { nullptr, 248.4f, 387.1f },
		{ nullptr, 261.2f, 386.3f }, { TEXT("Buona Vista") }, { nullptr, 282.8f, 392.8f }, { nullptr, 291.8f, 399.7f }, { TEXT("Commonwealth") },
		{ TEXT("Queenstown") }, { TEXT("Redhill") }, { nullptr, 344.1f, 449.7f }, { nullptr, 350.4f, 456.8f }, { TEXT("Tiong Bahru") },
		{ nullptr, 366.4f, 468.6f }, { nullptr, 376.2f, 473.1f }, { nullptr, 386.2f, 478.0f }, { TEXT("Outram Park") }, { nullptr, 403.0f, 494.0f },
		{ nullptr, 410.1f, 505.9f }, { nullptr, 416.6f, 517.3f }, { TEXT("Tanjong Pagar") }, { nullptr, 429.8f, 530.7f }, { nullptr, 437.7f, 533.6f },
		{ nullptr, 448.8f, 535.5f }, { nullptr, 462.7f, 536.5f }, { nullptr, 476.8f, 536.4f }, { nullptr, 488.3f, 534.8f }, { nullptr, 499.5f, 530.6f },
		{ nullptr, 506.6f, 524.5f }, { TEXT("Raffles Place") }, { nullptr, 506.2f, 503.7f }, { nullptr, 505.8f, 493.0f }, { TEXT("City Hall") },
		{ nullptr, 512.8f, 474.4f }, { TEXT("Bugis") }, { TEXT("Lavender") }, { TEXT("Kallang") }, { TEXT("Aljunied") }, { TEXT("Paya Lebar") },
		{ TEXT("Eunos") }, { TEXT("Kembangan") }, { nullptr, 707.3f, 275.6f }, { nullptr, 713.3f, 271.7f }, { nullptr, 720.1f, 269.5f }, { TEXT("Bedok") },
		{ nullptr, 751.0f, 269.2f }, { nullptr, 764.6f, 270.1f }, { nullptr, 778.5f, 270.7f }, { nullptr, 790.5f, 270.6f }, { TEXT("Tanah Merah") },
		{ nullptr, 802.5f, 262.2f }, { TEXT("Simei") }, { TEXT("Tampines") }, { TEXT("Pasir Ris") },
	};
	const FShapePoint GEW1[] = {
		{ TEXT("Tanah Merah") }, { nullptr, 809.8f, 274.4f }, { nullptr, 811.0f, 288.4f }, { nullptr, 817.1f, 298.8f }, { nullptr, 828.1f, 302.4f },
		{ TEXT("Expo") }, { TEXT("Changi Airport") },
	};
	const FShapePoint GNE0[] = {
		{ TEXT("HarbourFront") }, { nullptr, 377.7f, 554.4f }, { nullptr, 377.2f, 540.2f }, { nullptr, 377.1f, 523.8f }, { nullptr, 377.9f, 511.1f },
		{ nullptr, 380.0f, 501.9f }, { nullptr, 383.4f, 494.6f }, { TEXT("Outram Park") }, { TEXT("Chinatown") }, { TEXT("Clarke Quay") },
		{ nullptr, 440.7f, 435.3f }, { nullptr, 445.1f, 425.2f }, { TEXT("Dhoby Ghaut") }, { nullptr, 462.1f, 408.6f }, { nullptr, 467.8f, 403.3f },
		{ nullptr, 472.5f, 396.4f }, { nullptr, 475.2f, 385.3f }, { nullptr, 476.3f, 371.6f }, { TEXT("Little India") }, { TEXT("Farrer Park") },
		{ TEXT("Boon Keng") }, { nullptr, 476.1f, 296.8f }, { nullptr, 477.4f, 290.2f }, { nullptr, 483.6f, 283.3f }, { TEXT("Potong Pasir") },
		{ TEXT("Woodleigh") }, { TEXT("Serangoon") }, { TEXT("Kovan") }, { TEXT("Hougang") }, { TEXT("Buangkok") }, { TEXT("Sengkang") },
		{ nullptr, 624.5f, 135.2f }, { nullptr, 633.9f, 125.1f }, { nullptr, 644.3f, 114.6f }, { nullptr, 654.5f, 104.9f }, { TEXT("Punggol") },
		{ TEXT("Punggol Coast") },
	};
	const FShapePoint GCC0[] = {
		{ TEXT("Marina Bay") }, { nullptr, 526.8f, 551.9f }, { nullptr, 537.9f, 547.7f }, { nullptr, 549.5f, 543.4f }, { nullptr, 560.7f, 538.6f },
		{ TEXT("Bayfront") }, { nullptr, 580.6f, 525.5f }, { nullptr, 589.6f, 517.0f }, { nullptr, 597.9f, 508.2f }, { TEXT("Promenade") },
		{ nullptr, 613.5f, 489.6f }, { nullptr, 620.7f, 479.6f }, { TEXT("Nicoll Highway") }, { nullptr, 631.4f, 460.5f }, { nullptr, 635.2f, 451.4f },
		{ TEXT("Stadium") }, { nullptr, 641.1f, 432.5f }, { nullptr, 643.1f, 422.6f }, { TEXT("Mountbatten") }, { TEXT("Dakota") },
		{ nullptr, 644.9f, 373.4f }, { nullptr, 644.3f, 361.5f }, { TEXT("Paya Lebar") }, { nullptr, 640.2f, 338.9f }, { nullptr, 636.9f, 328.1f },
		{ nullptr, 633.0f, 317.4f }, { TEXT("MacPherson") }, { nullptr, 622.7f, 295.6f }, { nullptr, 615.9f, 284.6f }, { TEXT("Tai Seng") },
		{ nullptr, 600.9f, 265.5f }, { nullptr, 592.6f, 257.4f }, { TEXT("Bartley") }, { nullptr, 575.0f, 242.3f }, { nullptr, 566.0f, 235.2f },
		{ nullptr, 556.4f, 228.4f }, { TEXT("Serangoon") }, { nullptr, 535.3f, 217.3f }, { nullptr, 523.6f, 212.8f }, { nullptr, 511.6f, 209.0f },
		{ TEXT("Lorong Chuan") }, { nullptr, 488.9f, 204.1f }, { nullptr, 478.0f, 202.7f }, { nullptr, 467.1f, 201.7f }, { nullptr, 456.2f, 201.4f },
		{ TEXT("Bishan") }, { nullptr, 433.1f, 203.2f }, { nullptr, 420.7f, 205.5f }, { nullptr, 408.7f, 208.6f }, { TEXT("Marymount") },
		{ nullptr, 386.0f, 216.4f }, { nullptr, 375.5f, 221.3f }, { TEXT("Caldecott") }, { nullptr, 355.6f, 234.6f }, { nullptr, 346.0f, 242.7f },
		{ nullptr, 336.2f, 251.7f }, { nullptr, 326.6f, 261.1f }, { nullptr, 317.8f, 270.5f }, { TEXT("Botanic Gardens") }, { nullptr, 303.2f, 288.6f },
		{ nullptr, 297.2f, 297.9f }, { nullptr, 292.1f, 307.1f }, { TEXT("Farrer Road") }, { nullptr, 283.0f, 327.9f }, { nullptr, 279.5f, 339.5f },
		{ TEXT("Holland Village") }, { nullptr, 274.8f, 363.5f }, { nullptr, 273.7f, 375.9f }, { TEXT("Buona Vista") }, { nullptr, 272.5f, 397.5f },
		{ nullptr, 272.4f, 406.9f }, { TEXT("one-north") }, { TEXT("Kent Ridge") }, { TEXT("Haw Par Villa") }, { TEXT("Pasir Panjang") },
		{ TEXT("Labrador Park") }, { TEXT("Telok Blangah") }, { nullptr, 352.1f, 542.5f }, { nullptr, 360.3f, 549.1f }, { nullptr, 368.9f, 555.3f },
		{ TEXT("HarbourFront") }, { nullptr, 387.6f, 564.2f }, { nullptr, 397.8f, 566.9f }, { nullptr, 408.2f, 568.9f }, { TEXT("Keppel") },
		{ nullptr, 429.9f, 573.1f }, { nullptr, 441.4f, 574.8f }, { TEXT("Cantonment") }, { nullptr, 465.2f, 575.8f }, { nullptr, 477.4f, 575.1f },
		{ TEXT("Prince Edward Road") }, { nullptr, 498.4f, 568.2f }, { nullptr, 507.1f, 562.2f }, { TEXT("Marina Bay") },
	};
	const FShapePoint GCC1[] = {
		{ TEXT("Dhoby Ghaut") }, { nullptr, 455.7f, 416.0f }, { nullptr, 468.3f, 414.6f }, { nullptr, 480.4f, 415.9f }, { nullptr, 488.2f, 420.5f },
		{ nullptr, 495.5f, 427.6f }, { nullptr, 502.7f, 435.8f }, { TEXT("Bras Basah") }, { TEXT("Esplanade") }, { nullptr, 558.3f, 494.3f },
		{ nullptr, 565.9f, 503.9f }, { nullptr, 573.8f, 511.1f }, { nullptr, 584.2f, 516.0f }, { nullptr, 593.9f, 517.1f }, { nullptr, 600.0f, 513.8f },
		{ nullptr, 604.2f, 508.6f }, { TEXT("Promenade") },
	};
	const FShapePoint GDT0[] = {
		{ TEXT("Bukit Panjang") }, { nullptr, 245.0f, 135.9f }, { TEXT("Cashew") }, { nullptr, 259.6f, 152.4f }, { TEXT("Hillview") }, { TEXT("Hume") },
		{ TEXT("Beauty World") }, { TEXT("King Albert Park") }, { TEXT("Sixth Avenue") }, { nullptr, 262.6f, 265.8f }, { TEXT("Tan Kah Kee") },
		{ nullptr, 284.0f, 278.0f }, { TEXT("Botanic Gardens") }, { TEXT("Stevens") }, { nullptr, 379.2f, 279.9f }, { nullptr, 388.0f, 281.7f },
		{ nullptr, 396.8f, 285.3f }, { nullptr, 403.6f, 291.1f }, { nullptr, 410.3f, 298.8f }, { TEXT("Newton") }, { TEXT("Little India") },
		{ TEXT("Rochor") }, { TEXT("Bugis") }, { nullptr, 590.2f, 472.6f }, { nullptr, 597.0f, 479.9f }, { nullptr, 601.8f, 486.6f }, { TEXT("Promenade") },
		{ nullptr, 605.8f, 506.8f }, { nullptr, 604.2f, 514.1f }, { nullptr, 598.5f, 522.6f }, { nullptr, 590.2f, 529.3f }, { TEXT("Bayfront") },
		{ nullptr, 555.5f, 543.4f }, { nullptr, 543.0f, 544.9f }, { TEXT("Downtown") }, { nullptr, 519.2f, 544.8f }, { nullptr, 509.7f, 543.4f },
		{ nullptr, 502.7f, 538.4f }, { nullptr, 496.3f, 531.8f }, { TEXT("Telok Ayer") }, { nullptr, 470.4f, 506.3f }, { nullptr, 460.2f, 496.6f },
		{ nullptr, 449.0f, 486.1f }, { nullptr, 438.1f, 476.1f }, { nullptr, 428.4f, 467.5f }, { TEXT("Chinatown") }, { nullptr, 412.1f, 456.1f },
		{ nullptr, 408.4f, 443.9f }, { nullptr, 410.9f, 434.2f }, { TEXT("Fort Canning") }, { nullptr, 427.9f, 428.3f }, { nullptr, 440.1f, 428.3f },
		{ nullptr, 454.3f, 428.8f }, { nullptr, 468.7f, 429.4f }, { nullptr, 481.7f, 429.7f }, { nullptr, 491.4f, 429.3f }, { nullptr, 506.6f, 425.6f },
		{ TEXT("Bencoolen") }, { TEXT("Jalan Besar") }, { TEXT("Bendemeer") }, { TEXT("Geylang Bahru") }, { TEXT("Mattar") }, { TEXT("MacPherson") },
		{ TEXT("Ubi") }, { TEXT("Kaki Bukit") }, { TEXT("Bedok North") }, { nullptr, 706.1f, 237.8f }, { nullptr, 716.3f, 235.6f },
		{ TEXT("Bedok Reservoir") }, { TEXT("Tampines West") }, { TEXT("Tampines") }, { nullptr, 818.0f, 233.7f }, { nullptr, 829.9f, 234.7f },
		{ nullptr, 840.9f, 241.4f }, { TEXT("Tampines East") }, { TEXT("Upper Changi") }, { TEXT("Expo") }, { TEXT("Xilin") }, { TEXT("Sungei Bedok") },
	};
	const FShapePoint GTE0[] = {
		{ TEXT("Woodlands North") }, { TEXT("Woodlands") }, { TEXT("Woodlands South") }, { TEXT("Springleaf") }, { TEXT("Lentor") }, { TEXT("Mayflower") },
		{ nullptr, 354.8f, 148.7f }, { TEXT("Bright Hill") }, { nullptr, 364.5f, 175.5f }, { TEXT("Upper Thomson") }, { TEXT("Caldecott") },
		{ TEXT("Stevens") }, { TEXT("Napier") }, { TEXT("Orchard Boulevard") }, { TEXT("Orchard") }, { nullptr, 370.3f, 359.6f },
		{ nullptr, 367.0f, 368.8f }, { TEXT("Great World") }, { TEXT("Havelock") }, { nullptr, 364.2f, 414.4f }, { nullptr, 364.0f, 424.0f },
		{ nullptr, 365.1f, 433.0f }, { nullptr, 370.0f, 444.1f }, { nullptr, 376.7f, 454.9f }, { nullptr, 381.9f, 464.9f }, { nullptr, 387.6f, 475.2f },
		{ TEXT("Outram Park") }, { nullptr, 403.1f, 489.4f }, { nullptr, 412.7f, 493.3f }, { nullptr, 422.6f, 497.1f }, { TEXT("Maxwell") },
		{ nullptr, 439.7f, 508.5f }, { nullptr, 447.3f, 516.2f }, { nullptr, 454.4f, 523.8f }, { nullptr, 460.9f, 530.6f }, { TEXT("Shenton Way") },
		{ nullptr, 484.3f, 549.5f }, { nullptr, 492.6f, 552.5f }, { nullptr, 504.2f, 554.9f }, { TEXT("Marina Bay") }, { nullptr, 552.4f, 555.6f },
		{ nullptr, 563.2f, 555.7f }, { nullptr, 575.2f, 556.2f }, { nullptr, 587.3f, 556.5f }, { nullptr, 598.4f, 556.3f }, { nullptr, 607.3f, 555.0f },
		{ nullptr, 615.8f, 551.3f }, { nullptr, 622.5f, 546.4f }, { TEXT("Gardens by the Bay") }, { TEXT("Tanjong Rhu") }, { TEXT("Katong Park") },
		{ TEXT("Tanjong Katong") }, { TEXT("Marine Parade") }, { TEXT("Marine Terrace") }, { TEXT("Siglap") }, { TEXT("Bayshore") },
		{ TEXT("Bedok South") }, { nullptr, 815.4f, 355.3f }, { nullptr, 824.6f, 348.7f }, { nullptr, 833.0f, 343.9f }, { TEXT("Sungei Bedok") },
	};

	const FShapeRow GShapes[] = {
		{ 0, GNS0, UE_ARRAY_COUNT(GNS0) },
		{ 1, GEW0, UE_ARRAY_COUNT(GEW0) },
		{ 1, GEW1, UE_ARRAY_COUNT(GEW1) },
		{ 2, GNE0, UE_ARRAY_COUNT(GNE0) },
		{ 3, GCC0, UE_ARRAY_COUNT(GCC0) },
		{ 3, GCC1, UE_ARRAY_COUNT(GCC1) },
		{ 4, GDT0, UE_ARRAY_COUNT(GDT0) },
		{ 5, GTE0, UE_ARRAY_COUNT(GTE0) },
	};

	TArray<FKioskStation> BuildStations()
	{
		TArray<FKioskStation> Result;
		for (const FStationRow& Row : GRows)
		{
			FKioskStation Station;
			Station.Name = Row.Name;
			Station.LineMask = Row.Lines;
			Station.MapPos = FVector2f(Row.X, Row.Y);
			Result.Add(Station);
		}
		return Result;
	}

	/** Everything drawn or routed for one line: shapes (stations and bends), the station-only paths and the length of each hop. */
	struct FLineGeometry
	{
		TArray<TArray<FKioskPathPoint>> Shapes;
		TArray<TArray<int32>> Paths;
		TArray<TArray<float>> Hops; // Hops[Path][i] joins Paths[Path][i] and [i + 1], measured along the bends
	};

	const FLineGeometry& Geometry(int32 LineIndex)
	{
		static TArray<FLineGeometry> Cache;
		if (Cache.IsEmpty())
		{
			const TArray<FKioskStation>& Stations = FKioskData::Stations();
			Cache.SetNum(FKioskData::NumLines);
			for (const FShapeRow& Row : GShapes)
			{
				FLineGeometry& Geo = Cache[Row.Line];
				TArray<FKioskPathPoint>& Shape = Geo.Shapes.AddDefaulted_GetRef();
				TArray<int32>& Path = Geo.Paths.AddDefaulted_GetRef();
				TArray<float>& Hops = Geo.Hops.AddDefaulted_GetRef();
				float Length = 0.f;
				for (int32 Index = 0; Index < Row.Num; ++Index)
				{
					const FShapePoint& Point = Row.Points[Index];
					FKioskPathPoint Out;
					if (Point.Station)
					{
						Out.Station = FKioskData::FindStation(Point.Station);
						check(Out.Station != INDEX_NONE);
						Out.Pos = Stations[Out.Station].MapPos;
					}
					else
					{
						Out.Pos = FVector2f(Point.X, Point.Y);
					}
					if (!Shape.IsEmpty())
					{
						Length += FVector2f::Distance(Shape.Last().Pos, Out.Pos);
					}
					if (Out.Station != INDEX_NONE)
					{
						if (!Path.IsEmpty())
						{
							Hops.Add(Length);
						}
						Path.Add(Out.Station);
						Length = 0.f;
					}
					Shape.Add(Out);
				}
			}
		}
		return Cache[FMath::Clamp(LineIndex, 0, FKioskData::NumLines - 1)];
	}
}

const TArray<FKioskStation>& FKioskData::Stations()
{
	static const TArray<FKioskStation> Data = BuildStations();
	return Data;
}

int32 FKioskData::FindStation(const FString& Name)
{
	static TMap<FString, int32> Lookup;
	if (Lookup.IsEmpty())
	{
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(GRows); ++Index)
		{
			Lookup.Add(GRows[Index].Name, Index);
		}
	}
	const int32* Found = Lookup.Find(Name);
	return Found ? *Found : INDEX_NONE;
}

int32 FKioskData::CityHallIndex()
{
	static const int32 Index = FindStation(OriginName());
	return Index;
}

const FString& FKioskData::OriginName()
{
	static const FString Name = TEXT("City Hall");
	return Name;
}

const TArray<FKioskCashDenom>& FKioskData::CashDenominations()
{
	static const TArray<FKioskCashDenom> Data = {
		{ 200, false, TEXT("$2") }, { 500, false, TEXT("$5") }, { 1000, false, TEXT("$10") }, { 5000, false, TEXT("$50") },
		{ 10, true, TEXT("10¢") }, { 20, true, TEXT("20¢") }, { 50, true, TEXT("50¢") }, { 100, true, TEXT("$1") } };
	return Data;
}

bool FKioskData::IsAcceptedCash(int32 Cents)
{
	for (const FKioskCashDenom& Denom : CashDenominations())
	{
		if (Denom.Cents == Cents)
		{
			return true;
		}
	}
	return false;
}

const TArray<int32>& FKioskData::QuickTopUpAmounts()
{
	static const TArray<int32> Data = { 1000, 2000, 5000 };
	return Data;
}

const TArray<int32>& FKioskData::TopUpAmounts()
{
	static const TArray<int32> Data = { 1000, 2000, 3000, 5000, 10000 };
	return Data;
}

const TArray<int32>& FKioskData::PopularStations()
{
	static const TArray<int32> Data = {
		FindStation(TEXT("HarbourFront")), FindStation(TEXT("Bayfront")), FindStation(TEXT("Changi Airport")),
		FindStation(TEXT("Orchard")), FindStation(TEXT("Chinatown")) };
	return Data;
}

int32 FKioskData::PathService(int32 LineIndex, int32 PathIndex)
{
	// The Circle Line's Promenade - Esplanade - Dhoby Ghaut spur is part of the same service as the loop.
	return (LineIndex == 3) ? 0 : PathIndex;
}

const TArray<TArray<int32>>& FKioskData::LinePaths(int32 LineIndex)
{
	return Geometry(LineIndex).Paths;
}

const TArray<TArray<FKioskPathPoint>>& FKioskData::LineShapes(int32 LineIndex)
{
	return Geometry(LineIndex).Shapes;
}

const TArray<float>& FKioskData::HopLengths(int32 LineIndex, int32 PathIndex)
{
	return Geometry(LineIndex).Hops[PathIndex];
}

const TArray<FKioskConcession>& FKioskData::Concessions()
{
	static const TArray<FKioskConcession> Data = {
		{ TEXT("Bus"), TEXT("All bus services"), 2900, 9000, true, false },
		{ TEXT("Train"), TEXT("All MRT and LRT lines"), 2900, 9000, false, true },
		{ TEXT("Bus + Train"), TEXT("Every bus and train"), 5500, 12800, true, true },
	};
	return Data;
}

const FKioskCardInfo& FKioskData::Card(EKioskCardType Type)
{
	static const FKioskCardInfo Student = { EKioskCardType::Student, TEXT("4821"), 320, true, true, 2, 14 };
	static const FKioskCardInfo Adult = { EKioskCardType::Adult, TEXT("7305"), 1850, false, false, INDEX_NONE, 0 };
	return Type == EKioskCardType::Adult ? Adult : Student;
}

FText FKioskData::ConcessionName(int32 Index)
{
	switch (Index)
	{
	case 0: return KT("Bus");
	case 1: return KT("Train");
	default: return KT("Bus + Train");
	}
}

FText FKioskData::ConcessionDescription(int32 Index)
{
	switch (Index)
	{
	case 0: return KT("All bus services");
	case 1: return KT("All MRT and LRT lines");
	default: return KT("Every bus and train");
	}
}

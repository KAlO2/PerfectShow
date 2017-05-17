package com.cloudream.ishow.util;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.graphics.PointF;

public class MakeupDatabaseHelper extends SQLiteOpenHelper
{
	// If you change the database schema, you must increment the database version.
    public static final int DATABASE_VERSION = 1;
    public static final String DATABASE_NAME = "makeup.db";
    
	public MakeupDatabaseHelper(Context context)
	{
		 super(context, DATABASE_NAME, null, DATABASE_VERSION);
	}

	@Override
	public void onCreate(SQLiteDatabase db)
	{
//		db.execSQL(SQL_CREATE_ENTRIES);

	}

	@Override
	public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion)
	{
		// This database is only a cache for online data, so its upgrade policy is
        // to simply to discard the data and start over
//		db.execSQL(SQL_DELETE_ENTRIES);
		onCreate(db);
	}

	public static PointF[] getFeaturePoints(int i)
	{
		final int count = 81;
		PointF[] points = new PointF[count];
		
		points[0 ] = new PointF(131.000000f, 368.000000f);
		points[1 ] = new PointF(139.000000f, 433.000000f);
		points[2 ] = new PointF(153.000000f, 495.000000f);
		points[3 ] = new PointF(181.000000f, 564.000000f);
		points[4 ] = new PointF(228.000000f, 617.000000f);
		points[5 ] = new PointF(275.000000f, 660.000000f);
		points[6 ] = new PointF(322.000000f, 671.000000f);
		points[7 ] = new PointF(374.000000f, 657.000000f);
		points[8 ] = new PointF(425.000000f, 608.000000f);
		points[9 ] = new PointF(469.000000f, 553.000000f);
		points[10] = new PointF(492.000000f, 489.000000f);
		points[11] = new PointF(504.000000f, 430.000000f);
		points[12] = new PointF(511.000000f, 367.000000f);
		points[13] = new PointF(494.542480f, 298.744537f);
		points[14] = new PointF(454.795197f, 240.866959f);
		points[15] = new PointF(397.000000f, 201.000000f);
		points[16] = new PointF(311.000000f, 184.000000f);
		points[17] = new PointF(227.000000f, 198.000000f);
		points[18] = new PointF(177.082001f, 243.235016f);
		points[19] = new PointF(143.957413f, 301.893127f);
		points[20] = new PointF(239.000000f, 303.000000f);
		points[21] = new PointF(199.000000f, 299.000000f);
		points[22] = new PointF(164.000000f, 322.000000f);
		points[23] = new PointF(201.000000f, 319.000000f);
		points[24] = new PointF(237.000000f, 324.000000f);
		points[25] = new PointF(278.000000f, 327.000000f);
		points[26] = new PointF(360.000000f, 329.000000f);
		points[27] = new PointF(402.000000f, 305.000000f);
		points[28] = new PointF(444.000000f, 302.000000f);
		points[29] = new PointF(479.000000f, 329.000000f);
		points[30] = new PointF(442.000000f, 323.000000f);
		points[31] = new PointF(403.000000f, 326.000000f);
		points[32] = new PointF(406.000000f, 346.000000f);
		points[33] = new PointF(233.000000f, 343.000000f);
		points[34] = new PointF(269.000000f, 373.000000f);
		points[35] = new PointF(251.000000f, 361.000000f);
		points[36] = new PointF(234.000000f, 357.000000f);
		points[37] = new PointF(218.000000f, 361.000000f);
		points[38] = new PointF(202.000000f, 369.000000f);
		points[39] = new PointF(218.000000f, 381.000000f);
		points[40] = new PointF(235.000000f, 385.000000f);
		points[41] = new PointF(252.000000f, 381.000000f);
		points[42] = new PointF(237.000000f, 368.000000f);
		points[43] = new PointF(406.000000f, 371.000000f);
		points[44] = new PointF(372.000000f, 375.000000f);
		points[45] = new PointF(389.000000f, 365.000000f);
		points[46] = new PointF(407.000000f, 359.000000f);
		points[47] = new PointF(423.000000f, 363.000000f);
		points[48] = new PointF(439.000000f, 373.000000f);
		points[49] = new PointF(423.000000f, 383.000000f);
		points[50] = new PointF(407.000000f, 388.000000f);
		points[51] = new PointF(389.000000f, 385.000000f);
		points[52] = new PointF(345.000000f, 443.000000f);
		points[53] = new PointF(318.000000f, 443.000000f);
		points[54] = new PointF(291.000000f, 443.000000f);
		points[55] = new PointF(290.000000f, 493.000000f);
		points[56] = new PointF(317.000000f, 483.000000f);
		points[57] = new PointF(345.000000f, 494.000000f);
		points[58] = new PointF(369.000000f, 479.000000f);
		points[59] = new PointF(353.000000f, 497.000000f);
		points[60] = new PointF(317.000000f, 510.000000f);
		points[61] = new PointF(282.000000f, 496.000000f);
		points[62] = new PointF(267.000000f, 478.000000f);
		points[63] = new PointF(256.000000f, 549.000000f);
		points[64] = new PointF(280.000000f, 540.000000f);
		points[65] = new PointF(303.000000f, 537.000000f);
		points[66] = new PointF(318.000000f, 538.000000f);
		points[67] = new PointF(332.000000f, 537.000000f);
		points[68] = new PointF(356.000000f, 540.000000f);
		points[69] = new PointF(381.000000f, 549.000000f);
		points[70] = new PointF(346.000000f, 552.000000f);
		points[71] = new PointF(318.000000f, 553.000000f);
		points[72] = new PointF(291.000000f, 551.000000f);
		points[73] = new PointF(291.000000f, 559.000000f);
		points[74] = new PointF(318.000000f, 566.000000f);
		points[75] = new PointF(347.000000f, 559.000000f);
		points[76] = new PointF(366.000000f, 570.000000f);
		points[77] = new PointF(345.000000f, 583.000000f);
		points[78] = new PointF(319.000000f, 588.000000f);
		points[79] = new PointF(293.000000f, 584.000000f);
		points[80] = new PointF(272.000000f, 571.000000f);
		
		return points;
	}
}

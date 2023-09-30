package in.celest.xash3d;

import android.app.Activity;
import android.app.AlertDialog;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.content.Intent;
import android.widget.EditText;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.Button;
import android.widget.TextView;
import android.content.ComponentName;
import android.content.pm.PackageManager;
import android.content.SharedPreferences;

import in.celest.xash3d.schl.R;

public class LauncherActivity extends Activity {
	static EditText cmdArgs;
	static SharedPreferences mPref;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
	super.onCreate(savedInstanceState);
        // Build layout
	setContentView(R.layout.activity_launcher);
        cmdArgs = (EditText)findViewById(R.id.cmdArgs);
		cmdArgs.setSingleLine(true);
		mPref = getSharedPreferences("mod", 0);
		cmdArgs.setText(mPref.getString("argv","-console")); 
	}

	public void startXash(View view)
	{
		String argv = cmdArgs.getText().toString();
		Intent intent = new Intent();
		intent.setAction("in.celest.xash3d.START");
		intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

		SharedPreferences.Editor editor = mPref.edit();
		editor.putString("argv", argv);
		editor.commit();
		editor.apply();
		if(cmdArgs.length() != 0) intent.putExtra("argv", argv);
		// Uncomment to set gamedir here
		intent.putExtra("gamedir", "xschl");
		intent.putExtra("gamelibdir", getFilesDir().getAbsolutePath().replace("/files","/lib"));

		PackageManager pm = getPackageManager();
		ComponentName cn = intent.resolveActivity(pm);
		if(cn != null)
		{
			String packageName = cn.getPackageName();
			String versionName;
			try{
				versionName = pm.getPackageInfo(packageName, 0).versionName;
			}catch(PackageManager.NameNotFoundException e){
				showXashErrorDialog(e.toString());
				return;
			}
			if(0 <= versionName.compareTo("0.19.2"))
				startActivity(intent);
			else
				showXashUpdateDialog();
		}
		else
		{
			showXashInstallDialog("Xash3D FWGS ");
		}
	}

	public void showXashDialog(String title, String msg)
	{
		AlertDialog.Builder builder = new AlertDialog.Builder(this);

		builder.setTitle(title)
		.setMessage(msg)
		.show();
	}

	public void showXashErrorDialog(String msg)
	{
		showXashDialog("Xash Error", msg);
	}

	public void showXashInstallDialog(String msg)
	{
		showXashErrorDialog(msg + getString(R.string.alert_install_dialog_text));
	}

	public void showXashUpdateDialog()
	{
		showXashErrorDialog(getString(R.string.alert_update_dialog_text));
	}
}

package io.github.freefrank.lostodyssey;

import android.app.Activity;
import android.app.AlertDialog;
import android.widget.ArrayAdapter;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;

final class TvSettingsDialog {
    private TvSettingsDialog() {}
    static void show(Activity activity, TvOutputController output) {
        LinearLayout content = new LinearLayout(activity);
        content.setOrientation(LinearLayout.VERTICAL);
        int padding = Math.round(20 * activity.getResources().getDisplayMetrics().density);
        content.setPadding(padding, padding, padding, padding);
        TextView status = new TextView(activity);
        status.setText(output.status() + "\n\n" + activity.getString(R.string.tv_explanation));
        content.addView(status);
        Spinner mode = choice(activity, content, R.string.tv_mode, R.array.tv_modes, output.mode());
        Spinner resolution = choice(activity, content, R.string.tv_resolution, R.array.tv_resolutions,
            output.height() == 720 ? 0 : 1);
        ScrollView scroll = new ScrollView(activity);
        scroll.addView(content);
        new AlertDialog.Builder(activity).setTitle(R.string.tv_mode).setView(scroll)
            .setPositiveButton(R.string.tv_apply, (dialog, which) -> {
                output.preferences.edit().putInt("mode", mode.getSelectedItemPosition())
                    .putInt("height", resolution.getSelectedItemPosition() == 0 ? 720 : 1080).apply();
                output.settingsChanged();
            }).setNegativeButton(android.R.string.cancel, null).show();
    }
    private static Spinner choice(Activity activity, LinearLayout content, int label, int values, int selected) {
        TextView title = new TextView(activity);
        title.setText(label);
        content.addView(title);
        Spinner spinner = new Spinner(activity);
        ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(activity, values, android.R.layout.simple_spinner_item);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinner.setAdapter(adapter);
        spinner.setSelection(selected);
        content.addView(spinner);
        return spinner;
    }
}

package com.mojang.minecraftpe;

import com.mojang.android.StringValue;
import com.mojang.minecraftpe.R;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.TextView;
import android.widget.ToggleButton;

public class WorldTypeButton extends ToggleButton implements OnClickListener, StringValue {
	public WorldTypeButton(Context context, AttributeSet attrs) {
		super(context, attrs);
		_init();
	}

	//@Override
	public void onClick(View v) {
		_update();
	}
	@Override
	protected void onFinishInflate() {
		super.onFinishInflate();
		_update();
	}
	@Override
	protected void onAttachedToWindow() {
		if (!_attached) {
			_update();
			_attached = true;
		}
	}
	private boolean _attached = false;

	private void _init() {
		setOnClickListener(this);
	}
	private void _update() {
		_setWorldType(isChecked() ? Infinite : Old);
	}
	private void _setWorldType(int i) {
		_type = i;

		int id = R.string.worldtype_old_summary;
		if (_type == Infinite)
			id = R.string.worldtype_infinite_summary;
		String desc = getContext().getString(id);

		View v = getRootView().findViewById(R.id.labelWorldTypeDesc);
		if (desc != null && v != null && v instanceof TextView) {
			((TextView)v).setText(desc);
		}
	}

	public String getStringValue() {
		return (_type == Infinite) ? "infinite" : "old";
	}

	private int _type = Old;
	static final int Old      = 0;
	static final int Infinite = 1;
}

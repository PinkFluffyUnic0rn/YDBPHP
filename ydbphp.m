ydbget(var)
	quit @var
ydbset(var,val)
	set @var=val
	quit
ydbkill(var)
	kill @var
	quit
ydbdata(var)
	quit $data(@var)
ydborder(var,dir)
	quit $order(@var,dir)


class functions {	

	public static int systemMillisecs() {		
		return (int) (DateTime.Now.Ticks / TimeSpan.TicksPerMillisecond);
	}

}

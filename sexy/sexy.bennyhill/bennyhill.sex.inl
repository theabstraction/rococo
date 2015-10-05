namespace
{
	using namespace Sexy;
	using namespace Sexy::Sex;

	void Throw(ParseException& ex)
	{	
		TripDebugger();
		throw ex;
	}

	void Throw(cr_sex e, csexstr message)
	{
		SEXCHAR specimen[64];
		GetSpecimen(specimen, e);
		ParseException ex(e.Start(), e.End(), e.Tree().Source().Name(), message, specimen, &e);
		Throw(ex);
	}	
}
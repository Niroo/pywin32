// PyCOMTest.cpp : Implementation of CConnectApp and DLL registration.

#include "preconn.h"
#include "PyCOMTest.h"
#include "PyCOMImpl.h"
#include "SimpleCounter.h"

/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CPyCOMTest::InterfaceSupportsErrorInfo(REFIID riid)
{
	if (riid == IID_IPyCOMTest)
		return S_OK;
	return S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
//

DWORD WINAPI PyCOMTestSessionThreadEntry(void* pv)
{
	// Init COM for the thread.
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	CPyCOMTest::PyCOMTestSessionData* pS = (CPyCOMTest::PyCOMTestSessionData*)pv;
	// Unmarshal the interface pointer.
	IPyCOMTest *pi;
	HRESULT hr = CoGetInterfaceAndReleaseStream(pS->pStream, IID_IPyCOMTest, (void **)&pi);
	CComPtr<IPyCOMTest> p(pi);
	while (WaitForSingleObject(pS->m_hEvent, 0) != WAIT_OBJECT_0)
		p->Fire(pS->m_nID);
	p.Release();
	CoUninitialize();
	return 0;
}


CPyCOMTest::~CPyCOMTest()
{
	if (pLastArray) {
		SafeArrayDestroy(pLastArray);
		pLastArray = NULL;
	}
	StopAll();
}

void CPyCOMTest::CreatePyCOMTestSession(PyCOMTestSessionData& rs)
{
	DWORD dwThreadID = 0;
	_ASSERTE(rs.m_hEvent == NULL);
	_ASSERTE(rs.m_hThread == NULL);
	_ASSERTE(rs.pStream == NULL);
	rs.m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	HRESULT hr = CoMarshalInterThreadInterfaceInStream(IID_IPyCOMTest, (IPyCOMTest *)this, &rs.pStream );
	_ASSERTE(SUCCEEDED(hr) && rs.pStream != NULL);
	rs.m_hThread = CreateThread(NULL, 0, &PyCOMTestSessionThreadEntry, &rs, 0, &dwThreadID);
}

STDMETHODIMP CPyCOMTest::Start(long* pnID)
{
	if (pnID == NULL)
		return E_POINTER;
	*pnID = 0;
	HRESULT hRes = S_OK;
	m_cs.Lock();
	for (long i=0;i<nMaxSessions;i++)
	{
		if (m_rsArray[i].m_hEvent == NULL)
		{
			m_rsArray[i].m_nID = i;
			CreatePyCOMTestSession(m_rsArray[i]);
			*pnID = i;
			break;
		}
	}
	if (i == nMaxSessions) //fell through
		hRes = E_FAIL;
	m_cs.Unlock();
	return hRes;
}


STDMETHODIMP CPyCOMTest::Stop(long nID)
{
	HRESULT hRes = S_OK;
	m_cs.Lock();
	if (m_rsArray[nID].m_hEvent != NULL)
	{
		SetEvent(m_rsArray[nID].m_hEvent);
		WaitForSingleObject(m_rsArray[nID].m_hThread, INFINITE);
		CloseHandle(m_rsArray[nID].m_hThread);
		memset(&m_rsArray[nID], 0, sizeof(PyCOMTestSessionData));
	}
	else
		hRes = E_INVALIDARG;
	m_cs.Unlock();
	return hRes;
}

STDMETHODIMP CPyCOMTest::StopAll()
{
	m_cs.Lock();
	for (long i=0;i<nMaxSessions;i++)
	{
		if (m_rsArray[i].m_hEvent != NULL)
		{
			SetEvent(m_rsArray[i].m_hEvent);
			WaitForSingleObject(m_rsArray[i].m_hThread, INFINITE);
			CloseHandle(m_rsArray[i].m_hThread);
			memset(&m_rsArray[i], 0, sizeof(PyCOMTestSessionData));
		}
	}
	m_cs.Unlock();
	return S_OK;
}

//////////////////////
//
// My test specific stuff
//
STDMETHODIMP CPyCOMTest::Test(VARIANT, QsBoolean in, QsBoolean *out)
{
	*out = in;
	return S_OK;
}
STDMETHODIMP CPyCOMTest::Test2(QsAttribute in, QsAttribute *out)
{
	*out = in;
	return S_OK;
}
STDMETHODIMP CPyCOMTest::Test3(TestAttributes1 in, TestAttributes1* out)
{
	*out = in;
	return S_OK;
}
STDMETHODIMP CPyCOMTest::Test4(TestAttributes2 in,TestAttributes2* out)
{
	*out = in;
	return S_OK;
}

STDMETHODIMP CPyCOMTest::GetSetInterface(IPyCOMTest *ininterface, IPyCOMTest **outinterface)
{
	if (outinterface==NULL) return E_POINTER;
	*outinterface = ininterface;
	// Looks like I should definately AddRef() :-)
	ininterface->AddRef();
	return S_OK;
}

STDMETHODIMP CPyCOMTest::GetMultipleInterfaces(IPyCOMTest **outinterface1, IPyCOMTest **outinterface2)
{
	if (outinterface1==NULL || outinterface2==NULL) return E_POINTER;
	*outinterface1 = this;
	*outinterface2 = this;
	InternalAddRef(); // ??? Correct call?  AddRef fails compile...
	InternalAddRef();
	return S_OK;
}

STDMETHODIMP CPyCOMTest::GetSetDispatch(IDispatch *indisp, IDispatch **outdisp)
{
	*outdisp = indisp;
	indisp->AddRef();
	return S_OK;
}


STDMETHODIMP CPyCOMTest::GetSetUnknown(IUnknown *inunk, IUnknown **outunk)
{
	*outunk = inunk;
	inunk->AddRef();
	return S_OK;
}

STDMETHODIMP CPyCOMTest::SetIntSafeArray(SAFEARRAY* ints, int *resultSize)
{
	TCHAR buf[256];
	UINT cDims = SafeArrayGetDim(ints);
	*resultSize = 0;
	long ub=0, lb=0;
	if (cDims) {
		SafeArrayGetUBound(ints, 1, &ub);
		SafeArrayGetLBound(ints, 1, &lb);
		*resultSize = ub - lb + 1;
	}
	wsprintf(buf, _T("Have VARIANT SafeArray with %d dims and size %d\n"), cDims, *resultSize);
	OutputDebugString(buf);
	return S_OK;
}

STDMETHODIMP CPyCOMTest::SetVariantSafeArray(SAFEARRAY* vars, int *resultSize)
{
	TCHAR buf[256];
	UINT cDims = SafeArrayGetDim(vars);
	*resultSize = 0;
	long ub=0, lb=0;
	if (cDims) {
		SafeArrayGetUBound(vars, 1, &ub);
		SafeArrayGetLBound(vars, 1, &lb);
		*resultSize = ub - lb + 1;
	}
	wsprintf(buf, _T("Have VARIANT SafeArray with %d dims and size %d\n"), cDims, *resultSize);
	OutputDebugString(buf);
	return S_OK;
}

static HRESULT MakeFillIntArray(SAFEARRAY **ppRes, int len, VARENUM vt)
{
	HRESULT hr = S_OK;
	SAFEARRAY *psa;
	SAFEARRAYBOUND rgsabound[1] = { len, 0 };
	psa = SafeArrayCreate(VT_I4, 1, rgsabound);
	if (psa==NULL)
		return E_OUTOFMEMORY;
	long i;
	for (i=0;i<len;i++) {
		if (S_OK!=(hr=SafeArrayPutElement(psa, &i, &i))) {
			SafeArrayDestroy(psa);
			return hr;
		}
	}
	*ppRes = psa;
	return S_OK;
	}

STDMETHODIMP CPyCOMTest::GetSafeArrays(SAFEARRAY** attrs,
                                      SAFEARRAY**attrs2,
                                      SAFEARRAY** ints)
{
	HRESULT hr;
	*attrs = *attrs2 = *ints = NULL;
	if (S_OK != (hr=MakeFillIntArray(attrs, 5, VT_I4)))
		return hr;
	if (S_OK != (hr=MakeFillIntArray(attrs2, 10, VT_I4))) {
		SafeArrayDestroy(*attrs);
		return hr;
	}
	if (S_OK != (hr=MakeFillIntArray(ints, 20, VT_I4))) {
		SafeArrayDestroy(*attrs);
		SafeArrayDestroy(*attrs2);
		return hr;
	}
	return S_OK;
}

STDMETHODIMP CPyCOMTest::GetSimpleSafeArray(SAFEARRAY** attrs)
{
	return MakeFillIntArray(attrs, 10, VT_I4);
}

STDMETHODIMP CPyCOMTest::GetSimpleCounter(ISimpleCounter** counter)
{
	if (counter==NULL) return E_POINTER;
	typedef CComObject<CSimpleCounter> CCounter;

	*counter = new CCounter();
	(*counter)->AddRef();
	if (*counter==NULL) return E_OUTOFMEMORY;
	return S_OK;
}

STDMETHODIMP CPyCOMTest::SetVarArgs(SAFEARRAY *vararg)
{
	if (pLastArray) {
		SafeArrayDestroy(pLastArray);
		pLastArray = NULL;
	}
	return SafeArrayCopy(vararg, &pLastArray);
}

STDMETHODIMP CPyCOMTest::GetLastVarArgs(SAFEARRAY **result)
{
	if (result==NULL) return E_POINTER;
	if (!pLastArray)
		return E_FAIL;
	return SafeArrayCopy(pLastArray, result);
}

HRESULT CPyCOMTest::Fire(long nID)
{
	Lock();
	HRESULT hr = S_OK;
	IUnknown** pp = m_vec.begin();
	while (pp < m_vec.end() && hr == S_OK)
	{
		if (*pp != NULL)
		{
			CComQIPtr<IDispatch> pEvent = *pp;
			DISPID dispid;
			OLECHAR *names[] = { L"OnFire" };
			HRESULT hr = pEvent->GetIDsOfNames(IID_NULL, names, 1, 0, &dispid);
			if (SUCCEEDED(hr)) {
				CComVariant v(nID);
				DISPPARAMS params = { &v, NULL, 1, 0 };
				pEvent->Invoke(dispid, IID_NULL, 0, DISPATCH_METHOD, &params, NULL, NULL, NULL);
			}
//			IPyCOMTestEvent* pIEvent = (IPyCOMTestEvent*)*pp;
//			hr = pIEvent->Fire(nID);
		}
		pp++;
	}
	Unlock();
	return hr;
}

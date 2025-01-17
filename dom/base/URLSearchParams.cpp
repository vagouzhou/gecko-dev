/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "URLSearchParams.h"
#include "mozilla/dom/URLSearchParamsBinding.h"
#include "mozilla/dom/EncodingUtils.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(URLSearchParams, mObservers)
NS_IMPL_CYCLE_COLLECTING_ADDREF(URLSearchParams)
NS_IMPL_CYCLE_COLLECTING_RELEASE(URLSearchParams)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(URLSearchParams)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

URLSearchParams::URLSearchParams()
{
  SetIsDOMBinding();
}

URLSearchParams::~URLSearchParams()
{
  DeleteAll();
}

JSObject*
URLSearchParams::WrapObject(JSContext* aCx)
{
  return URLSearchParamsBinding::Wrap(aCx, this);
}

/* static */ already_AddRefed<URLSearchParams>
URLSearchParams::Constructor(const GlobalObject& aGlobal,
                             const nsAString& aInit,
                             ErrorResult& aRv)
{
  nsRefPtr<URLSearchParams> sp = new URLSearchParams();
  sp->ParseInput(NS_ConvertUTF16toUTF8(aInit), nullptr);
  return sp.forget();
}

/* static */ already_AddRefed<URLSearchParams>
URLSearchParams::Constructor(const GlobalObject& aGlobal,
                             URLSearchParams& aInit,
                             ErrorResult& aRv)
{
  nsRefPtr<URLSearchParams> sp = new URLSearchParams();
  aInit.mSearchParams.EnumerateRead(CopyEnumerator, sp);
  return sp.forget();
}

void
URLSearchParams::ParseInput(const nsACString& aInput,
                            URLSearchParamsObserver* aObserver)
{
  // Remove all the existing data before parsing a new input.
  DeleteAll();

  nsACString::const_iterator start, end;
  aInput.BeginReading(start);
  aInput.EndReading(end);
  nsACString::const_iterator iter(start);

  while (start != end) {
    nsAutoCString string;

    if (FindCharInReadable('&', iter, end)) {
      string.Assign(Substring(start, iter));
      start = ++iter;
    } else {
      string.Assign(Substring(start, end));
      start = end;
    }

    if (string.IsEmpty()) {
      continue;
    }

    nsACString::const_iterator eqStart, eqEnd;
    string.BeginReading(eqStart);
    string.EndReading(eqEnd);
    nsACString::const_iterator eqIter(eqStart);

    nsAutoCString name;
    nsAutoCString value;

    if (FindCharInReadable('=', eqIter, eqEnd)) {
      name.Assign(Substring(eqStart, eqIter));

      ++eqIter;
      value.Assign(Substring(eqIter, eqEnd));
    } else {
      name.Assign(string);
    }

    nsAutoString decodedName;
    DecodeString(name, decodedName);

    nsAutoString decodedValue;
    DecodeString(value, decodedValue);

    AppendInternal(decodedName, decodedValue);
  }

  NotifyObservers(aObserver);
}

void
URLSearchParams::DecodeString(const nsACString& aInput, nsAString& aOutput)
{
  nsACString::const_iterator start, end;
  aInput.BeginReading(start);
  aInput.EndReading(end);

  nsCString unescaped;

  while (start != end) {
    // replace '+' with U+0020
    if (*start == '+') {
      unescaped.Append(' ');
      ++start;
      continue;
    }

    // Percent decode algorithm
    if (*start == '%') {
      nsACString::const_iterator first(start);
      ++first;

      nsACString::const_iterator second(first);
      ++second;

#define ASCII_HEX_DIGIT( x )    \
  ((x >= 0x41 && x <= 0x46) ||  \
   (x >= 0x61 && x <= 0x66) ||  \
   (x >= 0x30 && x <= 0x39))

#define HEX_DIGIT( x )              \
   (*x >= 0x30 && *x <= 0x39        \
     ? *x - 0x30                    \
     : (*x >= 0x41 && *x <= 0x46    \
        ? *x - 0x37                 \
        : *x - 0x57))

      if (first != end && second != end &&
          ASCII_HEX_DIGIT(*first) && ASCII_HEX_DIGIT(*second)) {
        unescaped.Append(HEX_DIGIT(first) * 16 + HEX_DIGIT(second));
        start = ++second;
        continue;

      } else {
        unescaped.Append('%');
        ++start;
        continue;
      }
    }

    unescaped.Append(*start);
    ++start;
  }

  ConvertString(unescaped, aOutput);
}

void
URLSearchParams::ConvertString(const nsACString& aInput, nsAString& aOutput)
{
  aOutput.Truncate();

  if (!mDecoder) {
    mDecoder = EncodingUtils::DecoderForEncoding("UTF-8");
    if (!mDecoder) {
      MOZ_ASSERT(mDecoder, "Failed to create a decoder.");
      return;
    }
  }

  nsACString::const_iterator iter;
  aInput.BeginReading(iter);

  int32_t inputLength = aInput.Length();
  int32_t outputLength = 0;

  nsresult rv = mDecoder->GetMaxLength(iter.get(), inputLength,
                                       &outputLength);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  const mozilla::fallible_t fallible = mozilla::fallible_t();
  nsAutoArrayPtr<char16_t> buf(new (fallible) char16_t[outputLength + 1]);
  if (!buf) {
    return;
  }

  rv = mDecoder->Convert(iter.get(), &inputLength, buf, &outputLength);
  if (NS_SUCCEEDED(rv)) {
    buf[outputLength] = 0;
    if (!aOutput.Assign(buf, outputLength, mozilla::fallible_t())) {
      aOutput.Truncate();
    }
  }
}

/* static */ PLDHashOperator
URLSearchParams::CopyEnumerator(const nsAString& aName,
                                nsTArray<nsString>* aArray,
                                void *userData)
{
  URLSearchParams* aSearchParams = static_cast<URLSearchParams*>(userData);

  nsTArray<nsString>* newArray = new nsTArray<nsString>();
  newArray->AppendElements(*aArray);

  aSearchParams->mSearchParams.Put(aName, newArray);
  return PL_DHASH_NEXT;
}

void
URLSearchParams::AddObserver(URLSearchParamsObserver* aObserver)
{
  MOZ_ASSERT(aObserver);
  MOZ_ASSERT(!mObservers.Contains(aObserver));
  mObservers.AppendElement(aObserver);
}

void
URLSearchParams::RemoveObserver(URLSearchParamsObserver* aObserver)
{
  MOZ_ASSERT(aObserver);
  mObservers.RemoveElement(aObserver);
}

void
URLSearchParams::RemoveObservers()
{
  mObservers.Clear();
}

void
URLSearchParams::Get(const nsAString& aName, nsString& aRetval)
{
  nsTArray<nsString>* array;
  if (!mSearchParams.Get(aName, &array)) {
    aRetval.Truncate();
    return;
  }

  aRetval.Assign(array->ElementAt(0));
}

void
URLSearchParams::GetAll(const nsAString& aName, nsTArray<nsString>& aRetval)
{
  nsTArray<nsString>* array;
  if (!mSearchParams.Get(aName, &array)) {
    return;
  }

  aRetval.AppendElements(*array);
}

void
URLSearchParams::Set(const nsAString& aName, const nsAString& aValue)
{
  nsTArray<nsString>* array;
  if (!mSearchParams.Get(aName, &array)) {
    array = new nsTArray<nsString>();
    array->AppendElement(aValue);
    mSearchParams.Put(aName, array);
  } else {
    array->ElementAt(0) = aValue;
  }

  NotifyObservers(nullptr);
}

void
URLSearchParams::Append(const nsAString& aName, const nsAString& aValue)
{
  AppendInternal(aName, aValue);
  NotifyObservers(nullptr);
}

void
URLSearchParams::AppendInternal(const nsAString& aName, const nsAString& aValue)
{
  nsTArray<nsString>* array;
  if (!mSearchParams.Get(aName, &array)) {
    array = new nsTArray<nsString>();
    mSearchParams.Put(aName, array);
  }

  array->AppendElement(aValue);
}

bool
URLSearchParams::Has(const nsAString& aName)
{
  return mSearchParams.Get(aName, nullptr);
}

void
URLSearchParams::Delete(const nsAString& aName)
{
  nsTArray<nsString>* array;
  if (!mSearchParams.Get(aName, &array)) {
    return;
  }

  mSearchParams.Remove(aName);

  NotifyObservers(nullptr);
}

void
URLSearchParams::DeleteAll()
{
  mSearchParams.Clear();
}

class MOZ_STACK_CLASS SerializeData
{
public:
  SerializeData()
    : mFirst(true)
  {}

  nsAutoString mValue;
  bool mFirst;

  void Serialize(const nsCString& aInput)
  {
    const unsigned char* p = (const unsigned char*) aInput.get();

    while (p && *p) {
      // ' ' to '+'
      if (*p == 0x20) {
        mValue.Append(0x2B);
      // Percent Encode algorithm
      } else if (*p == 0x2A || *p == 0x2D || *p == 0x2E ||
                 (*p >= 0x30 && *p <= 0x39) ||
                 (*p >= 0x41 && *p <= 0x5A) || *p == 0x5F ||
                 (*p >= 0x61 && *p <= 0x7A)) {
        mValue.Append(*p);
      } else {
        mValue.AppendPrintf("%%%X", *p);
      }

      ++p;
    }
  }
};

void
URLSearchParams::Serialize(nsAString& aValue) const
{
  SerializeData data;
  mSearchParams.EnumerateRead(SerializeEnumerator, &data);
  aValue.Assign(data.mValue);
}

/* static */ PLDHashOperator
URLSearchParams::SerializeEnumerator(const nsAString& aName,
                                     nsTArray<nsString>* aArray,
                                     void *userData)
{
  SerializeData* data = static_cast<SerializeData*>(userData);

  for (uint32_t i = 0, len = aArray->Length(); i < len; ++i) {
    if (data->mFirst) {
      data->mFirst = false;
    } else {
      data->mValue.Append('&');
    }

    data->Serialize(NS_ConvertUTF16toUTF8(aName));
    data->mValue.Append('=');
    data->Serialize(NS_ConvertUTF16toUTF8(aArray->ElementAt(i)));
  }

  return PL_DHASH_NEXT;
}

void
URLSearchParams::NotifyObservers(URLSearchParamsObserver* aExceptObserver)
{
  for (uint32_t i = 0; i < mObservers.Length(); ++i) {
    if (mObservers[i] != aExceptObserver) {
      mObservers[i]->URLSearchParamsUpdated(this);
    }
  }
}

} // namespace dom
} // namespace mozilla
